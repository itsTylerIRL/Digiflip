#pragma once

#include "../core/digiflip_roster.h"

#include <gui/canvas.h>
#include <gui/icon.h>
#include <stdbool.h>

typedef enum {
    DigiflipEffectMeat,
    DigiflipEffectProtein,
    DigiflipEffectHearts,
    DigiflipEffectSparkle,
} DigiflipEffect;

/* Character poses. A Digimon without a drawn frame for a pose shows its
   idle (base) sprite instead. Order matches tools/build_sprites.py. */
typedef enum {
    DigiflipPoseIdle,
    DigiflipPoseWalk,
    DigiflipPoseHappy,
    DigiflipPoseEat,
    DigiflipPoseRefuse,
    DigiflipPoseSleep,
    DigiflipPoseAttack,
    DigiflipPoseHurt,
    DigiflipPoseCount,
} DigiflipPose;

/* Character art lives on the SD card (one file per Digimon) and is loaded
   into a small cache only while it's on screen. A sprite id is a roster
   species id, or a Colosseum-only boss via DIGIFLIP_SPRITE_BOSS(). */
typedef uint16_t DigiflipSpriteId;
#define DIGIFLIP_SPRITE_BOSS(index) ((DigiflipSpriteId)(0x8000U | (index)))
#define DIGIFLIP_SPRITE_NONE        ((DigiflipSpriteId)0xFFFFU)

DigiflipSpriteId digiflip_opponent_sprite_id(const DigiflipColosseumOpponent* opponent);
/* Load a sprite into the cache (no-op when already cached). Main thread
   only: it reads the SD card. */
void digiflip_sprites_prepare(DigiflipSpriteId sprite);
/* Draw a cached character, 36x36 at (x, y), facing left unless `flipped`.
   Draws nothing if the sprite hasn't been prepared. Uses the current canvas
   color, so it also draws in white on the dark sleep screen. */
void digiflip_draw_character(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    DigiflipSpriteId sprite,
    DigiflipPose pose,
    bool flipped);
const Icon* digiflip_egg_sprite(DigiflipEgg egg);
const Icon* digiflip_effect_sprite(DigiflipEffect effect);
/* 12x12 home-screen icon: the 7 actions (0=status 1=food 2=train 3=battle
   4=clean 5=light 6=medicine), then 7=call, which is an indicator only. */
const Icon* digiflip_menu_icon(uint8_t action);

void digiflip_sprites_init(void);
void digiflip_sprites_free(void);
