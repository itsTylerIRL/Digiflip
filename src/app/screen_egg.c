#include "digiflip_app.h"

/* The living pet a new egg would replace, or NULL (first egg, after a
   death, or over a Copymon). */
static const DigiflipGame* digiflip_egg_replaces(const DigiflipApp* app) {
    if(!app->egg_select_from_home) return NULL;
    if(app->egg_slot == 1 && app->slot2_kind != DigiflipSlotPet) return NULL;
    const DigiflipGame* game = &app->pets[app->egg_slot];
    return game->alive ? game : NULL;
}

uint16_t digiflip_eggs_open(const DigiflipApp* app) {
    if(app->settings.cheat_all_eggs) return (uint16_t)((1U << DigiflipEggCount) - 1U);
    return digiflip_eggs_unlocked(&app->album, app->records.wins);
}

void digiflip_check_unlocks(DigiflipApp* app) {
    const uint16_t unlocked = digiflip_eggs_unlocked(&app->album, app->records.wins);
    const uint16_t fresh = unlocked & (uint16_t)~app->records.known_eggs;
    if(!fresh) return;
    for(uint8_t egg = 0; egg < DigiflipEggCount; egg++) {
        if(!(fresh & (1U << egg))) continue;
        digiflip_log_app_event(app, DigiflipEventEggUnlocked, egg, 0);
        if(!app->unlock_notice) app->unlock_notice = (uint8_t)(egg + 1U);
    }
    app->records.known_eggs |= fresh;
    digiflip_records_save(&app->records);
}

void digiflip_record_win(DigiflipApp* app) {
    if(app->records.wins < UINT16_MAX) app->records.wins++;
    digiflip_records_save(&app->records);
    digiflip_check_unlocks(app);
}

static bool digiflip_egg_is_open(const DigiflipApp* app, uint8_t egg) {
    return (digiflip_eggs_open(app) >> egg) & 1U;
}

/* A pet that died after 48h awake since its last evolution leaves a traited
   egg for its slot. */
static bool digiflip_egg_traited(const DigiflipApp* app) {
    const DigiflipGame* game = &app->pets[app->egg_slot];
    return !app->egg_select_from_home && !game->alive && game->trait_earned;
}

/* Knock out every other pixel: a locked egg looks unlit. */
static void digiflip_dim_box(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h) {
    canvas_set_color(canvas, ColorWhite);
    for(int16_t yy = 0; yy < h; yy++) {
        for(int16_t xx = yy & 1; xx < w; xx += 2) {
            canvas_draw_dot(canvas, x + xx, y + yy);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}

static void digiflip_draw_requirement(Canvas* canvas, const DigiflipApp* app) {
    char buffer[32];
    const DigiflipEggStatus status =
        digiflip_egg_status((DigiflipEgg)app->selected_egg, &app->album, app->records.wins);
    switch(status.kind) {
    case DigiflipUnlockChild:
        canvas_draw_str(canvas, 47, 39, "Raise any Digimon");
        canvas_draw_str(canvas, 47, 48, "to Child");
        return;
    case DigiflipUnlockWins:
        snprintf(buffer, sizeof(buffer), "Win %u battles", status.goal);
        break;
    case DigiflipUnlockAlbum:
        snprintf(buffer, sizeof(buffer), "Raise %u Digimon", status.goal);
        break;
    default:
        return;
    }
    canvas_draw_str(canvas, 47, 39, buffer);
    snprintf(buffer, sizeof(buffer), "Progress %u/%u", status.progress, status.goal);
    canvas_draw_str(canvas, 47, 48, buffer);
}

void digiflip_draw_egg_select(Canvas* canvas, const DigiflipApp* app) {
    char buffer[32];
    const DigiflipEggInfo* egg = &digiflip_eggs[app->selected_egg];
    const DigiflipSpecies* hatch = digiflip_species_get(egg->hatch_species);
    const Icon* sprite = digiflip_egg_sprite((DigiflipEgg)app->selected_egg);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(
        canvas,
        2,
        8,
        app->egg_slot             ? "SLOT 2 DIGI-EGG" :
        app->egg_select_from_home ? "SLOT 1 DIGI-EGG" :
                                    "SELECT DIGI-EGG");
    snprintf(buffer, sizeof(buffer), "%u / %u", app->selected_egg + 1U, DigiflipEggCount);
    canvas_draw_str_aligned(canvas, 126, 8, AlignRight, AlignBottom, buffer);
    canvas_draw_line(canvas, 0, 10, 127, 10);
    const bool open = digiflip_egg_is_open(app, app->selected_egg);
    if(sprite) canvas_draw_icon(canvas, 5, 14, sprite);
    if(!open) digiflip_dim_box(canvas, 5, 14, 36, 36);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 47, open ? 28 : 26, egg->name);
    canvas_set_font(canvas, FontSecondary);
    if(open) {
        snprintf(buffer, sizeof(buffer), "Hatches: %s", hatch ? hatch->name : "Unknown");
        canvas_draw_str(canvas, 47, 42, buffer);
    } else {
        digiflip_draw_requirement(canvas, app);
    }
    canvas_draw_line(canvas, 0, 52, 127, 52);
    const DigiflipGame* replaces = digiflip_egg_replaces(app);
    if(!open) {
        snprintf(buffer, sizeof(buffer), "Locked   < > browse");
    } else if(digiflip_egg_traited(app)) {
        snprintf(buffer, sizeof(buffer), "Traited egg!  OK select");
    } else if(replaces && app->egg_confirm) {
        snprintf(
            buffer,
            sizeof(buffer),
            "OK again: replace %.10s",
            digiflip_game_species_name(replaces));
    } else if(replaces) {
        snprintf(buffer, sizeof(buffer), "Replaces %.16s", digiflip_game_species_name(replaces));
    } else {
        snprintf(buffer, sizeof(buffer), "< > browse   OK select");
    }
    canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, buffer);
}

void digiflip_egg_select_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyLeft) {
        app->selected_egg = app->selected_egg == 0 ? DigiflipEggCount - 1U :
                                                     app->selected_egg - 1U;
        app->egg_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyRight) {
        app->selected_egg = (app->selected_egg + 1U) % DigiflipEggCount;
        app->egg_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyUp) {
        app->selected_egg = (app->selected_egg + DigiflipEggCount - 5U) % DigiflipEggCount;
        app->egg_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        app->selected_egg = (app->selected_egg + 5U) % DigiflipEggCount;
        app->egg_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyOk) {
        if(!digiflip_egg_is_open(app, app->selected_egg)) {
            digiflip_sound_play(app->notification, DigiflipSoundFailure);
            return;
        }
        if(digiflip_egg_replaces(app) && !app->egg_confirm) {
            app->egg_confirm = true;
            digiflip_sound_play(app->notification, DigiflipSoundFailure);
            return;
        }
        app->egg_confirm = false;
        const uint8_t slot = app->egg_slot;
        const bool traited = digiflip_egg_traited(app);
        digiflip_game_new_with_egg(
            &app->pets[slot],
            furi_hal_rtc_get_timestamp(),
            (DigiflipEgg)app->selected_egg,
            traited);
        app->album_last_species[slot] = DIGIFLIP_SPECIES_NONE;
        if(slot == 1) {
            digiflip_set_slot2(app, DigiflipSlotPet, DIGIFLIP_SPECIES_NONE);
            digiflip_set_view(app, DigiflipViewBoth);
        } else {
            digiflip_storage_save_slot(&app->pets[0], 0);
            digiflip_set_view(app, app->view);
        }
        app->egg_slot = 0;
        app->egg_select_from_home = false;
        app->pet_anim_phase = 0;
        app->views[0].reaction = DigiflipReactionNone;
        app->views[1].reaction = DigiflipReactionNone;
        app->clean_start_tick = 0;
        app->evo_anim_start_tick = 0;
        app->selected_action = DigiflipActionStatus;
        app->screen = DigiflipScreenHome;
        digiflip_sound_play(app->notification, DigiflipSoundHatch);
    } else if(input->key == InputKeyBack) {
        app->egg_confirm = false;
        if(app->egg_select_from_home) {
            app->egg_select_from_home = false;
            app->screen = DigiflipScreenSlot;
            app->egg_slot = 0;
        } else if(app->egg_slot == 1) {
            /* Backing out after slot 2's pet died just empties the slot. */
            digiflip_set_slot2(app, DigiflipSlotEmpty, DIGIFLIP_SPECIES_NONE);
            app->egg_slot = 0;
            app->screen = DigiflipScreenHome;
        } else {
            app->running = false;
        }
    }
}
