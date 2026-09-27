#include "digiflip_app.h"

static bool digiflip_reaction_is_medicine(DigiflipReaction reaction) {
    return reaction == DigiflipReactionHealed || reaction == DigiflipReactionNotHealed;
}

static void digiflip_draw_slot(
    Canvas* canvas,
    const DigiflipApp* app,
    uint8_t slot,
    int16_t x,
    int16_t y,
    DigiflipPose pose,
    bool flipped) {
    if(digiflip_slot_raised(app, slot) && app->pets[slot].stage == DigiflipStageEgg) {
        const Icon* egg = digiflip_egg_sprite((DigiflipEgg)app->pets[slot].egg_id);
        if(egg) canvas_draw_icon(canvas, x, y, egg);
        return;
    }
    digiflip_draw_character(canvas, x, y, digiflip_slot_species(app, slot), pose, flipped);
}

static void digiflip_draw_creature(Canvas* canvas, const DigiflipApp* app, uint8_t slot) {
    const DigiflipPetView* view = &app->views[slot];
    const bool raised = digiflip_slot_raised(app, slot);
    const DigiflipGame* game = &app->pets[slot];
    const bool is_egg = raised && game->stage == DigiflipStageEgg;
    const bool asleep = raised && digiflip_game_is_asleep(game);
    const uint32_t tick = furi_get_tick();
    const uint32_t elapsed = tick - app->reaction_started_tick;
    const DigiflipReaction reaction = view->reaction;
    /* Sprites face left; mirror them when walking right. */
    bool flipped = !is_egg && view->direction > 0;
    const bool step = app->pet_anim_phase == 1U;
    int16_t x = is_egg ? view->x + (int16_t)app->pet_anim_phase - 1 : view->x;
    int16_t y = DIGIFLIP_PET_Y;
    DigiflipPose pose = DigiflipPoseIdle;
    const bool cheering =
        reaction == DigiflipReactionCheer ||
        (reaction == DigiflipReactionHealed && elapsed >= DIGIFLIP_MEDICINE_APPROACH_TICKS) ||
        ((reaction == DigiflipReactionMeat || reaction == DigiflipReactionProtein) &&
         elapsed >= DIGIFLIP_REACTION_EAT_TICKS);

    if(reaction == DigiflipReactionRefuse) {
        flipped = ((elapsed / 120U) & 1U) != 0;
        pose = DigiflipPoseRefuse;
    } else if(cheering) {
        const uint8_t happy_phase = (elapsed / 140U) % 3U;
        y = happy_phase == 1U ? DIGIFLIP_PET_Y - 5 : DIGIFLIP_PET_Y - 1;
        pose = DigiflipPoseHappy;
    } else if(reaction == DigiflipReactionNotHealed && elapsed >= DIGIFLIP_MEDICINE_APPROACH_TICKS) {
        pose = DigiflipPoseHurt;
        x += ((elapsed / 80U) & 1U) ? 1 : -1;
    } else if(digiflip_reaction_is_medicine(reaction)) {
        pose = DigiflipPoseIdle;
    } else if(reaction != DigiflipReactionNone) {
        if(elapsed >= DIGIFLIP_REACTION_APPROACH_TICKS) {
            const uint32_t bite = elapsed - DIGIFLIP_REACTION_APPROACH_TICKS;
            pose = (bite % DIGIFLIP_BITE_TICKS) < DIGIFLIP_BITE_TICKS / 2U ? DigiflipPoseEat :
                                                                             DigiflipPoseIdle;
        }
    } else if(asleep) {
        pose = DigiflipPoseSleep;
    } else if(raised && digiflip_game_is_tired(game)) {
        pose = ((tick / 700U) & 1U) ? DigiflipPoseSleep : DigiflipPoseIdle;
    } else if(step) {
        pose = raised && digiflip_game_is_injured(game) ? DigiflipPoseHurt : DigiflipPoseWalk;
        y = DIGIFLIP_PET_Y - 1;
    }

    digiflip_draw_slot(canvas, app, slot, x, y, pose, flipped);
}

/* Effect icons are 20px with ink in columns 2-17. */
#define DIGIFLIP_FX_INK_LEFT 2
#define DIGIFLIP_FX_INK_W    16

static bool digiflip_fx_fits(const DigiflipApp* app, int16_t icon_x) {
    const int16_t left = icon_x + DIGIFLIP_FX_INK_LEFT;
    const int16_t right = left + DIGIFLIP_FX_INK_W;
    if(left < 0 || right > 128) return false;
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_visible(app, slot)) continue;
        const int16_t pet_x = app->views[slot].x;
        if(left < pet_x + DIGIFLIP_PET_WIDTH && right > pet_x) return false;
    }
    return true;
}

static void
    digiflip_draw_joy(Canvas* canvas, const DigiflipApp* app, uint8_t slot, uint32_t elapsed) {
    const int16_t pet_x = app->views[slot].x;
    const int16_t beside_right = pet_x + DIGIFLIP_PET_WIDTH - DIGIFLIP_FX_INK_LEFT + 1;
    const int16_t beside_left = pet_x - DIGIFLIP_FX_INK_LEFT - DIGIFLIP_FX_INK_W - 1;
    int16_t effect_x;
    if(digiflip_fx_fits(app, beside_right)) {
        effect_x = beside_right;
    } else if(digiflip_fx_fits(app, beside_left)) {
        effect_x = beside_left;
    } else {
        return;
    }
    const DigiflipEffect effect = ((elapsed / 220U) & 1U) ? DigiflipEffectSparkle :
                                                            DigiflipEffectHearts;
    canvas_draw_icon(canvas, effect_x, DIGIFLIP_PET_Y, digiflip_effect_sprite(effect));
}

static void digiflip_draw_reaction(Canvas* canvas, const DigiflipApp* app, uint8_t slot) {
    const DigiflipReaction reaction = app->views[slot].reaction;
    const int16_t pet_x = app->views[slot].x;
    if(reaction == DigiflipReactionNone || reaction == DigiflipReactionRefuse) return;

    const uint32_t elapsed = furi_get_tick() - app->reaction_started_tick;
    if(reaction == DigiflipReactionCheer) {
        digiflip_draw_joy(canvas, app, slot, elapsed);
        return;
    }
    if(digiflip_reaction_is_medicine(reaction)) {
        if(elapsed < DIGIFLIP_MEDICINE_APPROACH_TICKS) {
            const int16_t top = DIGIFLIP_FIELD_TOP;
            const int16_t land = DIGIFLIP_PET_Y + 8;
            const int16_t y = (int16_t)(top + (land - top) * (int32_t)elapsed /
                                                  (int32_t)DIGIFLIP_MEDICINE_APPROACH_TICKS);
            canvas_draw_icon(canvas, pet_x + 12, y, digiflip_menu_icon(DigiflipActionMedicine));
        } else if(reaction == DigiflipReactionHealed) {
            digiflip_draw_joy(canvas, app, slot, elapsed);
        } else {
            const int16_t sx = pet_x < 74 ? pet_x + DIGIFLIP_PET_WIDTH + 1 : pet_x - 5;
            const int16_t drop = (int16_t)((elapsed / 90U) % 8U);
            canvas_draw_dot(canvas, sx, DIGIFLIP_PET_Y + 6 + drop);
            canvas_draw_dot(canvas, sx + 3, DIGIFLIP_PET_Y + 2 + (drop + 4) % 8);
        }
        return;
    }
    if(elapsed < DIGIFLIP_REACTION_EAT_TICKS) {
        const DigiflipEffect food = reaction == DigiflipReactionMeat ? DigiflipEffectMeat :
                                                                       DigiflipEffectProtein;
        const int16_t target_x = pet_x + 32;
        const int16_t target_y = DIGIFLIP_PET_Y + 15;
        const bool drop = app->view == DigiflipViewBoth;
        const int16_t start = drop ? DIGIFLIP_FIELD_TOP - 2 : 106;
        const int16_t goal = drop ? target_y : target_x;
        int16_t along = goal;
        uint8_t bites = 0;
        if(elapsed < DIGIFLIP_REACTION_APPROACH_TICKS) {
            along = start + (int16_t)(((goal - start) * (int32_t)elapsed) /
                                      (int32_t)DIGIFLIP_REACTION_APPROACH_TICKS);
        } else {
            bites = (uint8_t)((elapsed - DIGIFLIP_REACTION_APPROACH_TICKS) / DIGIFLIP_BITE_TICKS);
        }
        const int16_t x = drop ? target_x : along;
        const int16_t y = drop ? along : target_y;
        canvas_draw_icon(canvas, x, y, digiflip_effect_sprite(food));
        if(bites) {
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_box(canvas, x, y, bites * 5U, 20);
            canvas_set_color(canvas, ColorBlack);
        }
    } else {
        digiflip_draw_joy(canvas, app, slot, elapsed - DIGIFLIP_REACTION_EAT_TICKS);
    }
}

void digiflip_start_reaction(DigiflipApp* app, uint8_t slot, DigiflipReaction reaction) {
    app->views[slot].reaction = reaction;
    app->views[slot].reaction_sound_played = false;
    app->reaction_started_tick = furi_get_tick();
    app->pet_anim_phase = 0;
}

static void digiflip_draw_evo(Canvas* canvas, const DigiflipApp* app) {
    const DigiflipGame* game = &app->pets[app->evo_slot];
    const uint32_t elapsed = furi_get_tick() - app->evo_anim_start_tick;
    const DigiflipSpeciesId from = digiflip_game_evo_from_species(game);
    const int16_t x = 46;
    const int16_t y = DIGIFLIP_PET_Y;
    /* 0 = nothing, 1 = old form, 2 = new form, 3 = new form celebrating. */
    uint8_t show = 0;
    if(elapsed < 800U) {
        if(((elapsed / 200U) & 1U) == 0) show = 1;
    } else if(elapsed < 2000U) {
        show = ((elapsed / 150U) & 1U) ? 2 : 1;
    } else {
        show = ((elapsed / 250U) & 1U) == 0 ? 3 : 2;
        const Icon* fx = digiflip_effect_sprite(DigiflipEffectSparkle);
        if(((elapsed / 250U) & 1U) == 0) {
            canvas_draw_icon(canvas, x - 22, y + 8, fx);
        } else {
            canvas_draw_icon(canvas, x + 38, y + 8, fx);
        }
    }
    if(show == 1 && from == DIGIFLIP_SPECIES_NONE) {
        const Icon* egg = digiflip_egg_sprite((DigiflipEgg)game->egg_id);
        if(egg) canvas_draw_icon(canvas, x, y, egg);
    } else if(show == 1) {
        digiflip_draw_character(canvas, x, y, from, DigiflipPoseIdle, false);
    } else if(show) {
        digiflip_draw_character(
            canvas,
            x,
            y,
            game->species_id,
            show == 3 ? DigiflipPoseHappy : DigiflipPoseIdle,
            false);
    }
    if(elapsed >= 2000U && app->evo_album_new) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, DIGIFLIP_FIELD_TOP + 9, "NEW!");
    }
    if(elapsed >= 800U && elapsed < 2000U && (elapsed % 400U) < 90U) {
        canvas_draw_box(canvas, 0, 0, 128, 64);
    }
}

static void digiflip_draw_evo_banner(Canvas* canvas, const DigiflipApp* app) {
    if(app->evo_anim_start_tick == 0 || furi_get_tick() - app->evo_anim_start_tick < 2000U) return;
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, DIGIFLIP_FIELD_BOTTOM, 128, 64 - DIGIFLIP_FIELD_BOTTOM);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 0, DIGIFLIP_FIELD_BOTTOM, 127, DIGIFLIP_FIELD_BOTTOM);
    const char* name = digiflip_game_species_name(&app->pets[app->evo_slot]);
    canvas_set_font(canvas, FontPrimary);
    if(canvas_string_width(canvas, name) > 124) canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, name);
}

static void digiflip_draw_poop(Canvas* canvas, int16_t x, int16_t y, bool steaming) {
    canvas_draw_box(canvas, x, y + 6, 8, 2);
    canvas_draw_box(canvas, x + 1, y + 4, 6, 2);
    canvas_draw_box(canvas, x + 2, y + 2, 4, 2);
    canvas_draw_dot(canvas, x + 4, y + 1);
    if(steaming) {
        canvas_draw_dot(canvas, x + 1, y - 1);
        canvas_draw_dot(canvas, x + 7, y);
    }
}

static void digiflip_draw_waste(Canvas* canvas, const DigiflipApp* app) {
    const bool flushing = app->clean_start_tick != 0;
    int16_t wave_x = 128;
    if(flushing) {
        const uint32_t elapsed = furi_get_tick() - app->clean_start_tick;
        wave_x = (int16_t)(128 - (int32_t)(elapsed * 140U / DIGIFLIP_CLEAN_ANIM_MS));
    }
    const bool steaming = ((furi_get_tick() / 400U) & 1U) != 0;
    uint8_t targets[2];
    const uint8_t count = digiflip_care_targets(app, targets);
    for(uint8_t t = 0; t < count; t++) {
        const uint8_t slot = targets[t];
        const uint8_t messes = flushing ? app->views[slot].clean_messes : app->pets[slot].messes;
        for(uint8_t i = 0; i < messes; i++) {
            const int16_t x = digiflip_poop_x(app, slot, i);
            if(x + 8 < wave_x) {
                digiflip_draw_poop(canvas, x, DIGIFLIP_FIELD_BOTTOM - 10, steaming);
            }
        }
    }
    if(flushing && wave_x > -4 && wave_x < 128) {
        for(int16_t y = DIGIFLIP_FIELD_TOP; y < DIGIFLIP_FIELD_BOTTOM; y++) {
            const int16_t wobble = (int16_t)(((y / 3) & 1) ? 1 : -1);
            canvas_draw_dot(canvas, wave_x + wobble, y);
            canvas_draw_dot(canvas, wave_x + wobble + 1, y);
            if((y % 6) == 0) canvas_draw_line(canvas, wave_x + 4, y, wave_x + 7, y + 2);
        }
    }
}

static void digiflip_dim_icon(Canvas* canvas, int16_t x, int16_t y) {
    canvas_set_color(canvas, ColorWhite);
    for(int16_t yy = 0; yy < DIGIFLIP_ICON_SIZE; yy++) {
        for(int16_t xx = (yy & 1); xx < DIGIFLIP_ICON_SIZE; xx += 2) {
            canvas_draw_dot(canvas, x + xx, y + yy);
        }
    }
    canvas_set_color(canvas, ColorBlack);
}

static bool digiflip_any_call(const DigiflipApp* app) {
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(digiflip_slot_raised(app, slot) &&
           digiflip_game_call_reason(&app->pets[slot]) != DigiflipCallNone)
            return true;
    }
    return false;
}

/* Four icons along the top edge, four along the bottom; the last bottom slot
   is the call icon, which lights up (blinking) when either pet calls. */
static void digiflip_draw_menu(Canvas* canvas, const DigiflipApp* app) {
    const bool calling = digiflip_any_call(app);
    const bool blink_on = ((furi_get_tick() / 500U) & 1U) == 0;
    for(uint8_t i = 0; i <= DIGIFLIP_CALL_SLOT; i++) {
        const int16_t x =
            (i % 4U) * DIGIFLIP_ICON_SLOT + (DIGIFLIP_ICON_SLOT - DIGIFLIP_ICON_SIZE) / 2;
        const int16_t y = i < 4 ? 0 : 64 - DIGIFLIP_ICON_SIZE;
        const Icon* icon = digiflip_menu_icon(i);
        if(!icon) continue;
        if(i == DIGIFLIP_CALL_SLOT) {
            if(calling && !blink_on) continue;
            canvas_draw_icon(canvas, x, y, icon);
            if(!calling) digiflip_dim_icon(canvas, x, y);
        } else if(i == app->selected_action) {
            canvas_draw_rbox(canvas, x - 2, y, DIGIFLIP_ICON_SIZE + 4, DIGIFLIP_ICON_SIZE, 2);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_icon(canvas, x, y, icon);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_draw_icon(canvas, x, y, icon);
        }
    }
}

void digiflip_draw_home(Canvas* canvas, const DigiflipApp* app) {
    uint8_t targets[2];
    const uint8_t count = digiflip_care_targets(app, targets);
    /* Lights out only when every pet on screen is asleep in the dark. */
    bool dark = count > 0;
    for(uint8_t i = 0; i < count; i++) {
        const DigiflipGame* game = &app->pets[targets[i]];
        if(!digiflip_game_is_asleep(game) || game->lights_on) dark = false;
    }
    canvas_set_font(canvas, FontSecondary);
    if(dark) {
        canvas_draw_box(
            canvas, 0, DIGIFLIP_FIELD_TOP, 128, DIGIFLIP_FIELD_BOTTOM - DIGIFLIP_FIELD_TOP);
        canvas_set_color(canvas, ColorWhite);
    }

    bool injured = false;
    if(app->evo_anim_start_tick != 0) {
        digiflip_draw_evo(canvas, app);
    } else {
        for(uint8_t slot = 0; slot < 2; slot++) {
            if(!digiflip_slot_visible(app, slot)) continue;
            digiflip_draw_creature(canvas, app, slot);
            digiflip_draw_reaction(canvas, app, slot);
            if(!digiflip_slot_raised(app, slot)) continue;
            const DigiflipGame* game = &app->pets[slot];
            if(digiflip_game_is_asleep(game)) {
                const int16_t pet_x = app->views[slot].x;
                const uint8_t phase = (uint8_t)((furi_get_tick() / 600U) % 3U);
                const int16_t zx = pet_x < 70 ? pet_x + 34 : pet_x - 10;
                for(uint8_t i = 0; i <= phase; i++) {
                    canvas_draw_str(canvas, zx + i * 6, DIGIFLIP_PET_Y + 16 - i * 6, "z");
                }
            } else if(digiflip_game_is_injured(game)) {
                injured = true;
            }
        }
    }
    if(injured && ((furi_get_tick() / 400U) & 1U) == 0) {
        /* A blinking bandage flags an injury. */
        canvas_draw_icon(
            canvas,
            128 - DIGIFLIP_ICON_SIZE - 2,
            DIGIFLIP_FIELD_TOP + 2,
            digiflip_menu_icon(DigiflipActionMedicine));
    }

    digiflip_draw_waste(canvas, app);
    canvas_set_color(canvas, ColorBlack);
    digiflip_draw_menu(canvas, app);
    digiflip_draw_evo_banner(canvas, app);
}

bool digiflip_home_busy(const DigiflipApp* app) {
    return app->views[0].reaction != DigiflipReactionNone ||
           app->views[1].reaction != DigiflipReactionNone || app->evo_anim_start_tick != 0 ||
           app->clean_start_tick != 0;
}

/* Targets that can act right now (hatched and awake); returns the count. */
static uint8_t digiflip_awake_targets(const DigiflipApp* app, uint8_t out[2]) {
    uint8_t all[2];
    const uint8_t count = digiflip_care_targets(app, all);
    uint8_t awake = 0;
    for(uint8_t i = 0; i < count; i++) {
        const DigiflipGame* game = &app->pets[all[i]];
        if(game->stage != DigiflipStageEgg && !digiflip_game_is_asleep(game))
            out[awake++] = all[i];
    }
    return awake;
}

static const char* digiflip_nobody_reason(const DigiflipApp* app) {
    uint8_t all[2];
    const uint8_t count = digiflip_care_targets(app, all);
    if(count == 0) return "A Copymon needs no care";
    for(uint8_t i = 0; i < count; i++) {
        if(app->pets[all[i]].stage != DigiflipStageEgg) return "Asleep...";
    }
    return "Wait for hatch!";
}

static void digiflip_do_light(DigiflipApp* app) {
    uint8_t targets[2];
    const uint8_t count = digiflip_care_targets(app, targets);
    const uint32_t now = furi_hal_rtc_get_timestamp();
    const char* notice = NULL;
    bool acted = false;
    for(uint8_t i = 0; i < count; i++) {
        DigiflipGame* game = &app->pets[targets[i]];
        if(game->stage == DigiflipStageEgg) continue;
        acted = true;
        if(digiflip_game_is_asleep(game) && game->lights_on) {
            /* It dozed off with the lights on: just darken the room. */
            digiflip_game_lights_off(game);
            digiflip_sound_play(app->notification, DigiflipSoundLightOff);
        } else if(digiflip_game_is_asleep(game)) {
            digiflip_game_wake(game, now);
            digiflip_sound_play(app->notification, DigiflipSoundLightOn);
            notice = "Good morning!";
        } else {
            const bool tired = digiflip_game_is_tired(game);
            digiflip_game_sleep(game, now);
            digiflip_sound_play(app->notification, DigiflipSoundLightOff);
            if(!notice) notice = tired ? "Good night..." : "Nap time...";
        }
    }
    if(!acted) {
        digiflip_refuse(app, count ? "Wait for hatch!" : "A Copymon needs no care");
    } else if(notice) {
        digiflip_show_notice(app, notice);
    }
}

static void digiflip_do_clean(DigiflipApp* app) {
    uint8_t targets[2];
    const uint8_t count = digiflip_care_targets(app, targets);
    bool any = false;
    for(uint8_t i = 0; i < count; i++) {
        const uint8_t slot = targets[i];
        app->views[slot].clean_messes = app->pets[slot].messes;
        if(digiflip_game_clean(&app->pets[slot])) any = true;
    }
    if(any) {
        app->clean_start_tick = furi_get_tick();
        digiflip_sound_play(app->notification, DigiflipSoundClean);
    } else {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
        digiflip_show_notice(app, count ? "Already clean" : "A Copymon needs no care");
    }
}

static void digiflip_do_medicine(DigiflipApp* app) {
    uint8_t targets[2];
    const uint8_t count = digiflip_awake_targets(app, targets);
    if(count == 0) {
        digiflip_refuse(app, digiflip_nobody_reason(app));
        return;
    }
    bool treated = false;
    for(uint8_t i = 0; i < count; i++) {
        const uint8_t slot = targets[i];
        DigiflipGame* game = &app->pets[slot];
        if(!digiflip_game_is_injured(game)) {
            digiflip_start_reaction(app, slot, DigiflipReactionRefuse);
        } else {
            treated = true;
            digiflip_start_reaction(
                app,
                slot,
                digiflip_game_heal(game) ? DigiflipReactionHealed : DigiflipReactionNotHealed);
        }
    }
    digiflip_sound_play(app->notification, treated ? DigiflipSoundClick : DigiflipSoundFailure);
}

void digiflip_handle_home(DigiflipApp* app, const InputEvent* input) {
    /* Hold input while a reaction, evolution or flush is playing. */
    if(digiflip_home_busy(app)) return;
    uint8_t awake[2];
    if(input->key == InputKeyUp || input->key == InputKeyDown) {
        uint8_t next = app->selected_action < 4 ? app->selected_action + 4U :
                                                  app->selected_action - 4U;
        if(next >= DigiflipActionCount) next = DigiflipActionCount - 1U;
        app->selected_action = next;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyLeft) {
        app->selected_action = app->selected_action == 0 ? DigiflipActionCount - 1U :
                                                           app->selected_action - 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyRight) {
        app->selected_action = (app->selected_action + 1U) % DigiflipActionCount;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        /* Never quit on a stray Back: open the pause menu instead. */
        app->pause_index = DigiflipPauseResume;
        app->screen = DigiflipScreenPause;
        digiflip_sound_play(app->notification, DigiflipSoundClick);
    } else if(input->key == InputKeyOk) {
        switch((DigiflipAction)app->selected_action) {
        case DigiflipActionStatus:
            if(!digiflip_care_targets(app, awake)) {
                digiflip_refuse(app, "A Copymon needs no care");
                break;
            }
            digiflip_sound_play(app->notification, DigiflipSoundClick);
            app->status_page = 0;
            app->status_from_slot = false;
            app->screen = DigiflipScreenStatus;
            break;
        case DigiflipActionFeed:
            if(!digiflip_awake_targets(app, awake)) {
                digiflip_refuse(app, digiflip_nobody_reason(app));
            } else {
                digiflip_sound_play(app->notification, DigiflipSoundClick);
                app->screen = DigiflipScreenFeed;
            }
            break;
        case DigiflipActionTrain:
        case DigiflipActionBattle:
            /* Either pet can go solo, or lead a tag team, whatever the view:
               a baby in one slot never benches the other. */
            digiflip_sound_play(app->notification, DigiflipSoundClick);
            digiflip_mode_open(app, (DigiflipAction)app->selected_action);
            break;
        case DigiflipActionClean:
            digiflip_do_clean(app);
            break;
        case DigiflipActionLight:
            digiflip_do_light(app);
            break;
        case DigiflipActionMedicine:
            digiflip_do_medicine(app);
            break;
        case DigiflipActionCount:
            break;
        }
    }
}

static uint32_t digiflip_reaction_length(DigiflipReaction reaction) {
    switch(reaction) {
    case DigiflipReactionRefuse:
        return DIGIFLIP_REFUSE_TICKS;
    case DigiflipReactionCheer:
        return DIGIFLIP_CHEER_TICKS;
    case DigiflipReactionHealed:
    case DigiflipReactionNotHealed:
        return DIGIFLIP_MEDICINE_TOTAL_TICKS;
    default:
        return DIGIFLIP_REACTION_TOTAL_TICKS;
    }
}

static void digiflip_reaction_sound(DigiflipApp* app, DigiflipPetView* view, uint32_t elapsed) {
    if(view->reaction_sound_played) return;
    DigiflipSound sound;
    switch(view->reaction) {
    case DigiflipReactionMeat:
    case DigiflipReactionProtein:
        if(elapsed < DIGIFLIP_REACTION_EAT_TICKS) return;
        sound = DigiflipSoundHappy;
        break;
    case DigiflipReactionCheer:
        sound = DigiflipSoundHappy;
        break;
    case DigiflipReactionHealed:
    case DigiflipReactionNotHealed:
        if(elapsed < DIGIFLIP_MEDICINE_APPROACH_TICKS) return;
        sound = view->reaction == DigiflipReactionHealed ? DigiflipSoundSuccess :
                                                           DigiflipSoundFailure;
        break;
    default:
        return;
    }
    view->reaction_sound_played = true;
    digiflip_sound_play(app->notification, sound);
}

void digiflip_update_pet_walk(DigiflipApp* app, uint32_t tick) {
    const uint32_t elapsed = tick - app->reaction_started_tick;
    bool reacting = false;
    for(uint8_t slot = 0; slot < 2; slot++) {
        DigiflipPetView* view = &app->views[slot];
        if(view->reaction == DigiflipReactionNone) continue;
        digiflip_reaction_sound(app, view, elapsed);
        if(elapsed >= digiflip_reaction_length(view->reaction)) {
            view->reaction = DigiflipReactionNone;
        } else {
            reacting = true;
        }
    }
    if(reacting) return;
    if(app->clean_start_tick != 0 && tick - app->clean_start_tick >= DIGIFLIP_CLEAN_ANIM_MS) {
        app->clean_start_tick = 0;
        /* A clean home is worth a cheer (unless it slept through it). */
        uint8_t awake[2];
        const uint8_t count = digiflip_awake_targets(app, awake);
        for(uint8_t i = 0; i < count; i++) {
            if(app->views[awake[i]].clean_messes) {
                digiflip_start_reaction(app, awake[i], DigiflipReactionCheer);
            }
        }
        return;
    }
    if(tick - app->last_pet_step_tick < DIGIFLIP_WALK_STEP_TICKS) return;
    app->last_pet_step_tick = tick;
    app->pet_anim_phase = (app->pet_anim_phase + 1U) % 3U;
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_visible(app, slot)) continue;
        /* Eggs wobble in place; sleepers and dozers stay put. */
        if(digiflip_slot_raised(app, slot)) {
            const DigiflipGame* game = &app->pets[slot];
            if(game->stage == DigiflipStageEgg || digiflip_game_is_asleep(game) ||
               digiflip_game_is_tired(game))
                continue;
        }
        DigiflipPetView* view = &app->views[slot];
        int16_t min_x;
        int16_t max_x;
        digiflip_lane(app, slot, &min_x, &max_x);
        if(view->direction == 0) view->direction = 1;
        view->x += view->direction;
        if(view->x >= max_x) {
            view->x = max_x;
            view->direction = -1;
        } else if(view->x <= min_x) {
            view->x = min_x;
            view->direction = 1;
        }
    }
}
