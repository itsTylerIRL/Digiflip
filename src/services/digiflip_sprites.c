#include "digiflip_sprites.h"

#include "digiflip_icons.h"

#include <furi.h>
#include <string.h>
#include <stdlib.h>
#include <storage/storage.h>
#include "digiflip_sprite_data.inc"

#define DIGIFLIP_DOT_SIZE           18U
#define DIGIFLIP_DOT_ROW_BYTES      3U
#define DIGIFLIP_FRAME_BYTES        (DIGIFLIP_DOT_SIZE * DIGIFLIP_DOT_ROW_BYTES)
#define DIGIFLIP_SPRITE_SIZE        (DIGIFLIP_DOT_SIZE * 2U)
#define DIGIFLIP_SPRITE_ROW_BYTES   ((DIGIFLIP_SPRITE_SIZE + 7U) / 8U)
/* Enough for a tag battle (pet, Copymon, two opponents) plus a previous form
   mid-evolution and one spare. */
#define DIGIFLIP_SPRITE_CACHE_SLOTS 6U

typedef struct {
    DigiflipSpriteId sprite;
    uint8_t mask; /* poses present; 0 = file missing (don't retry) */
    uint32_t last_used;
    uint8_t frames[DigiflipPoseCount][DIGIFLIP_FRAME_BYTES];
} DigiflipSpriteSlot;

static DigiflipSpriteSlot* digiflip_cache;
static uint32_t digiflip_cache_clock;
static uint8_t digiflip_scaled[DIGIFLIP_SPRITE_ROW_BYTES * DIGIFLIP_SPRITE_SIZE];

DigiflipSpriteId digiflip_opponent_sprite_id(const DigiflipColosseumOpponent* opponent) {
    if(!opponent) return DIGIFLIP_SPRITE_NONE;
    if(opponent->species != DIGIFLIP_SPECIES_NONE) return opponent->species;
    return DIGIFLIP_SPRITE_BOSS(opponent->boss);
}

static bool digiflip_sprite_path(DigiflipSpriteId sprite, FuriString* path) {
    if(sprite & 0x8000U) {
        const DigiflipColosseumOpponent* boss = digiflip_colosseum_boss((uint8_t)(sprite & 0xFFU));
        if(!boss) return false;
        furi_string_printf(path, "%s/cs_%s.dfs", APP_ASSETS_PATH("sprites"), boss->key);
        return true;
    }
    const DigiflipSpecies* species = digiflip_species_get(sprite);
    if(!species) return false;
    furi_string_printf(path, "%s/sp_%s.dfs", APP_ASSETS_PATH("sprites"), species->key);
    return true;
}

static void digiflip_sprite_load(DigiflipSpriteSlot* slot, DigiflipSpriteId sprite) {
    slot->sprite = sprite;
    slot->mask = 0;
    FuriString* path = furi_string_alloc();
    if(digiflip_sprite_path(sprite, path)) {
        Storage* storage = furi_record_open(RECORD_STORAGE);
        File* file = storage_file_alloc(storage);
        uint8_t header[6];
        if(storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING) &&
           storage_file_read(file, header, sizeof(header)) == sizeof(header) &&
           memcmp(header, "DFSP", 4) == 0 && header[4] == 1U && (header[5] & 1U)) {
            uint8_t mask = 0;
            for(uint8_t pose = 0; pose < DigiflipPoseCount; pose++) {
                if(!(header[5] & (1U << pose))) continue;
                if(storage_file_read(file, slot->frames[pose], DIGIFLIP_FRAME_BYTES) !=
                   DIGIFLIP_FRAME_BYTES)
                    break;
                mask |= (uint8_t)(1U << pose);
            }
            slot->mask = mask;
        }
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
    }
    furi_string_free(path);
}

static DigiflipSpriteSlot* digiflip_sprite_find(DigiflipSpriteId sprite) {
    if(!digiflip_cache) return NULL;
    for(uint8_t i = 0; i < DIGIFLIP_SPRITE_CACHE_SLOTS; i++) {
        if(digiflip_cache[i].sprite == sprite) return &digiflip_cache[i];
    }
    return NULL;
}

void digiflip_sprites_prepare(DigiflipSpriteId sprite) {
    if(!digiflip_cache || sprite == DIGIFLIP_SPRITE_NONE) return;
    DigiflipSpriteSlot* slot = digiflip_sprite_find(sprite);
    if(!slot) {
        /* Evict the least recently used slot. */
        slot = &digiflip_cache[0];
        for(uint8_t i = 1; i < DIGIFLIP_SPRITE_CACHE_SLOTS; i++) {
            if(digiflip_cache[i].last_used < slot->last_used) slot = &digiflip_cache[i];
        }
        digiflip_sprite_load(slot, sprite);
    }
    slot->last_used = ++digiflip_cache_clock;
}

/* Double each dot of an 18-row bitmap to 2x2 (mirrored if asked) into the
   XBM buffer. */
static void digiflip_scale_rows(const uint32_t* rows, bool flipped) {
    memset(digiflip_scaled, 0, sizeof(digiflip_scaled));
    for(uint8_t row = 0; row < DIGIFLIP_DOT_SIZE; row++) {
        uint8_t* top = digiflip_scaled + (row * 2U) * DIGIFLIP_SPRITE_ROW_BYTES;
        for(uint8_t col = 0; col < DIGIFLIP_DOT_SIZE; col++) {
            if(!(rows[row] & (1UL << col))) continue;
            const uint8_t dot = flipped ? (uint8_t)(DIGIFLIP_DOT_SIZE - 1U - col) : col;
            for(uint8_t half = 0; half < 2; half++) {
                const uint8_t px = (uint8_t)(dot * 2U + half);
                top[px / 8U] |= (uint8_t)(1U << (px % 8U));
            }
        }
        memcpy(top + DIGIFLIP_SPRITE_ROW_BYTES, top, DIGIFLIP_SPRITE_ROW_BYTES);
    }
}

void digiflip_draw_character(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    DigiflipSpriteId sprite,
    DigiflipPose pose,
    bool flipped) {
    const DigiflipSpriteSlot* slot = digiflip_sprite_find(sprite);
    if(!slot || !slot->mask) return;
    if(pose >= DigiflipPoseCount || !(slot->mask & (1U << pose))) pose = DigiflipPoseIdle;
    const uint8_t* frame = slot->frames[pose];
    const uint32_t full = (1UL << DIGIFLIP_DOT_SIZE) - 1U;
    uint32_t ink[DIGIFLIP_DOT_SIZE];
    uint32_t outside[DIGIFLIP_DOT_SIZE];
    for(uint8_t row = 0; row < DIGIFLIP_DOT_SIZE; row++) {
        const uint8_t* bytes = frame + row * DIGIFLIP_DOT_ROW_BYTES;
        ink[row] = ((uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16)) &
                   full;
        /* Seed the outside with the blank border dots... */
        const bool edge = row == 0 || row == DIGIFLIP_DOT_SIZE - 1U;
        const uint32_t border = edge ? full : (1UL | (1UL << (DIGIFLIP_DOT_SIZE - 1U)));
        outside[row] = border & ~ink[row];
    }
    /* ...and flood it inward through blank dots, so only the space outside
       the outline is see-through. */
    for(bool grew = true; grew;) {
        grew = false;
        for(uint8_t row = 0; row < DIGIFLIP_DOT_SIZE; row++) {
            uint32_t spread = outside[row] | (outside[row] << 1) | (outside[row] >> 1);
            if(row > 0) spread |= outside[row - 1U];
            if(row + 1U < DIGIFLIP_DOT_SIZE) spread |= outside[row + 1U];
            spread &= full & ~ink[row];
            if(spread != outside[row]) {
                outside[row] = spread;
                grew = true;
            }
        }
    }
    /* Fill the body in the paper colour, then draw the ink on top: whatever
       is behind shows around the character but never through it. */
    uint32_t body[DIGIFLIP_DOT_SIZE];
    for(uint8_t row = 0; row < DIGIFLIP_DOT_SIZE; row++) {
        body[row] = full & ~outside[row] & ~ink[row];
    }
    digiflip_scale_rows(body, flipped);
    canvas_invert_color(canvas);
    canvas_draw_xbm(canvas, x, y, DIGIFLIP_SPRITE_SIZE, DIGIFLIP_SPRITE_SIZE, digiflip_scaled);
    canvas_invert_color(canvas);
    digiflip_scale_rows(ink, flipped);
    canvas_draw_xbm(canvas, x, y, DIGIFLIP_SPRITE_SIZE, DIGIFLIP_SPRITE_SIZE, digiflip_scaled);
}

const Icon* digiflip_egg_sprite(DigiflipEgg egg) {
    if(egg >= DigiflipEggCount) return NULL;
    return digiflip_egg_sprites[egg];
}

const Icon* digiflip_effect_sprite(DigiflipEffect effect) {
    switch(effect) {
    case DigiflipEffectMeat:
        return &I_fx_meat;
    case DigiflipEffectProtein:
        return &I_fx_protein;
    case DigiflipEffectHearts:
        return &I_fx_hearts;
    case DigiflipEffectSparkle:
        return &I_fx_sparkle;
    default:
        return NULL;
    }
}

const Icon* digiflip_menu_icon(uint8_t action) {
    switch(action) {
    case 0:
        return &I_menu_status;
    case 1:
        return &I_menu_food;
    case 2:
        return &I_menu_train;
    case 3:
        return &I_menu_battle;
    case 4:
        return &I_menu_clean;
    case 5:
        return &I_menu_light;
    case 6:
        return &I_menu_medicine;
    case 7:
        return &I_menu_call;
    default:
        return NULL;
    }
}

void digiflip_sprites_init(void) {
    if(digiflip_cache) return;
    digiflip_cache = malloc(sizeof(DigiflipSpriteSlot) * DIGIFLIP_SPRITE_CACHE_SLOTS);
    furi_check(digiflip_cache);
    for(uint8_t i = 0; i < DIGIFLIP_SPRITE_CACHE_SLOTS; i++) {
        digiflip_cache[i].sprite = DIGIFLIP_SPRITE_NONE;
        digiflip_cache[i].mask = 0;
        digiflip_cache[i].last_used = 0;
    }
}

void digiflip_sprites_free(void) {
    free(digiflip_cache);
    digiflip_cache = NULL;
}
