#include "digiflip_app.h"

/* One line of the picker: who leads, and whether slot 2's partner (or pet 1,
   when pet 2 leads) joins as a tag team. */
typedef struct {
    uint8_t lead;
    bool tag;
} DigiflipPlayOption;

#define DIGIFLIP_PLAY_OPTIONS_MAX 4U

static uint8_t digiflip_play_options(const DigiflipApp* app, DigiflipPlayOption out[]) {
    const bool second_pet = app->slot2_kind == DigiflipSlotPet;
    uint8_t count = 0;
    out[count++] = (DigiflipPlayOption){0, false};
    if(second_pet) out[count++] = (DigiflipPlayOption){1, false};
    if(app->slot2_kind != DigiflipSlotEmpty) out[count++] = (DigiflipPlayOption){0, true};
    if(second_pet) out[count++] = (DigiflipPlayOption){1, true};
    return count;
}

/* Short reason a raised pet can't do this now, or NULL when it can. */
static const char* digiflip_play_block(const DigiflipGame* game, DigiflipAction action) {
    if(!game->alive) return "gone";
    if(game->stage == DigiflipStageEgg) return "egg";
    if(action == DigiflipActionTrain) return digiflip_game_is_asleep(game) ? "asleep" : NULL;
    switch(digiflip_game_battle_block(game)) {
    case DigiflipBattleTooYoung:
        return "young";
    case DigiflipBattleAsleep:
        return "asleep";
    case DigiflipBattleInjured:
        return "hurt";
    case DigiflipBattleNoDp:
        return "no DP";
    case DigiflipBattleOk:
    default:
        return NULL;
    }
}

/* Why an option can't be played (checking the lead, then the partner). A
   Copymon partner is always ready. */
static const char* digiflip_option_block(
    const DigiflipApp* app,
    DigiflipPlayOption option,
    DigiflipAction action,
    const char** who) {
    *who = digiflip_game_species_name(&app->pets[option.lead]);
    const char* blocked = digiflip_play_block(&app->pets[option.lead], action);
    if(blocked || !option.tag) return blocked;
    const uint8_t partner = (uint8_t)(1U - option.lead);
    if(!digiflip_slot_raised(app, partner)) return NULL;
    *who = digiflip_game_species_name(&app->pets[partner]);
    return digiflip_play_block(&app->pets[partner], action);
}

static const char* digiflip_slot_short_name(const DigiflipApp* app, uint8_t slot) {
    if(digiflip_slot_raised(app, slot)) return digiflip_game_species_name(&app->pets[slot]);
    const DigiflipSpecies* species = digiflip_species_get(digiflip_slot_species(app, slot));
    return species ? species->name : "?";
}

static void digiflip_mode_start(DigiflipApp* app, DigiflipPlayOption option);

void digiflip_mode_open(DigiflipApp* app, DigiflipAction action) {
    DigiflipPlayOption options[DIGIFLIP_PLAY_OPTIONS_MAX];
    const uint8_t count = digiflip_play_options(app, options);
    const char* who;
    /* Start on what's on screen: the pair side by side, else that pet solo;
       failing that, the first option that can play. */
    const bool want_tag = app->view == DigiflipViewBoth;
    const uint8_t want_lead = digiflip_focus_slot(app);
    app->mode_action = (uint8_t)action;
    app->mode_index = 0;
    if(count == 1) {
        digiflip_mode_start(app, options[0]);
        if(app->screen == DigiflipScreenNotice) app->notice_return = DigiflipScreenHome;
        return;
    }
    bool found = false;
    for(uint8_t i = 0; i < count && !found; i++) {
        if(options[i].tag == want_tag && options[i].lead == want_lead &&
           !digiflip_option_block(app, options[i], action, &who)) {
            app->mode_index = i;
            found = true;
        }
    }
    for(uint8_t i = 0; i < count && !found; i++) {
        if(!digiflip_option_block(app, options[i], action, &who)) {
            app->mode_index = i;
            found = true;
        }
    }
    app->screen = DigiflipScreenModePick;
}

void digiflip_draw_mode_pick(Canvas* canvas, const DigiflipApp* app) {
    DigiflipPlayOption options[DIGIFLIP_PLAY_OPTIONS_MAX];
    const uint8_t count = digiflip_play_options(app, options);
    char texts[DIGIFLIP_PLAY_OPTIONS_MAX][32];
    const char* labels[DIGIFLIP_PLAY_OPTIONS_MAX];
    const char* values[DIGIFLIP_PLAY_OPTIONS_MAX];
    for(uint8_t i = 0; i < count; i++) {
        const char* who;
        const char* blocked =
            digiflip_option_block(app, options[i], (DigiflipAction)app->mode_action, &who);
        const int width = blocked ? 6 : 8;
        const char* lead = digiflip_slot_short_name(app, options[i].lead);
        if(options[i].tag) {
            snprintf(
                texts[i],
                sizeof(texts[i]),
                "Tag %.*s+%.*s",
                width,
                lead,
                width,
                digiflip_slot_short_name(app, (uint8_t)(1U - options[i].lead)));
        } else {
            snprintf(texts[i], sizeof(texts[i]), "Solo %.*s", width + 6, lead);
        }
        labels[i] = texts[i];
        values[i] = blocked;
    }
    digiflip_draw_list(
        canvas,
        app->mode_action == DigiflipActionTrain ? "TRAIN" : "BATTLE",
        labels,
        values,
        count,
        app->mode_index);
}

static void digiflip_tag_train_start(DigiflipApp* app) {
    app->tag_shot = 0;
    app->tag_hits = 0;
    app->tag_shot_tick = 0;
    app->tag_guard_high = digiflip_game_tag_guard_high(app->game);
    app->screen = DigiflipScreenTagTrain;
}

/* Play the chosen option: the lead becomes the focused pet, so the partner
   is the other slot for the rest of the session. */
static void digiflip_mode_start(DigiflipApp* app, DigiflipPlayOption option) {
    const DigiflipAction action = (DigiflipAction)app->mode_action;
    const char* who;
    const char* blocked = digiflip_option_block(app, option, action, &who);
    if(blocked) {
        snprintf(app->notice_buffer, sizeof(app->notice_buffer), "%.24s", who);
        snprintf(
            app->notice_detail_buffer,
            sizeof(app->notice_detail_buffer),
            "can't %s: %s",
            action == DigiflipActionTrain ? "train" : "battle",
            blocked);
        digiflip_refuse(app, app->notice_buffer);
        app->notice_detail = app->notice_detail_buffer;
        app->notice_return = DigiflipScreenModePick;
        return;
    }
    digiflip_sound_play(app->notification, DigiflipSoundClick);
    app->game = &app->pets[option.lead];
    if(action == DigiflipActionTrain) {
        if(option.tag) {
            digiflip_tag_train_start(app);
        } else {
            digiflip_start_mash(app, DigiflipScreenTrain);
        }
    } else {
        app->battle_tag = option.tag;
        app->battle_sync = DigiflipSyncNear;
        app->mash_presses = 0;
        app->screen = DigiflipScreenBattleIntro;
    }
}

void digiflip_mode_input(DigiflipApp* app, const InputEvent* input) {
    DigiflipPlayOption options[DIGIFLIP_PLAY_OPTIONS_MAX];
    const uint8_t count = digiflip_play_options(app, options);
    if(app->mode_index >= count) app->mode_index = 0;
    if(input->key == InputKeyUp) {
        app->mode_index = app->mode_index == 0 ? count - 1U : app->mode_index - 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        app->mode_index = (uint8_t)((app->mode_index + 1U) % count);
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk) {
        digiflip_mode_start(app, options[app->mode_index]);
    }
}

#define DIGIFLIP_TAG_SHOT_MS 700U

void digiflip_draw_tag_train(Canvas* canvas, const DigiflipApp* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "TAG TRAIN");
    for(uint8_t i = 0; i < DIGIFLIP_TAG_TRAIN_ROUNDS; i++) {
        const int16_t x = 84 + i * 9;
        if(i < app->tag_shot) {
            if(i < 8 && (app->tag_hits >> i) & 1U) {
                canvas_draw_disc(canvas, x, 6, 3);
            } else {
                canvas_draw_line(canvas, x - 2, 4, x + 2, 8);
                canvas_draw_line(canvas, x - 2, 8, x + 2, 4);
            }
        } else {
            canvas_draw_circle(canvas, x, 6, 3);
        }
    }

    digiflip_draw_character(canvas, 24, 14, digiflip_mate_species(app), DigiflipPoseIdle, true);
    const bool shooting = app->tag_shot_tick != 0;
    digiflip_draw_character(
        canvas,
        2,
        14,
        app->game->species_id,
        shooting ? DigiflipPoseAttack : DigiflipPoseIdle,
        true);
    canvas_draw_frame(canvas, 104, 16, 10, 32);
    canvas_draw_line(canvas, 104, 32, 113, 32);

    canvas_set_font(canvas, FontSecondary);
    if(!shooting) {
        canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, "Up: high   Down: low");
        return;
    }
    const uint32_t elapsed = furi_get_tick() - app->tag_shot_tick;
    const int16_t guard_y = app->tag_guard_high ? 17 : 33;
    canvas_draw_box(canvas, 100, guard_y, 3, 14);
    const int16_t shot_y = app->tag_aim_high ? 24 : 40;
    const int16_t shot_x =
        (int16_t)(40 + (int32_t)60 * (int32_t)(elapsed < 350U ? elapsed : 350U) / 350);
    if(elapsed < 450U) canvas_draw_disc(canvas, shot_x, shot_y, 2);
    const bool hit = app->tag_aim_high != app->tag_guard_high;
    if(elapsed >= 350U) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(
            canvas, 64, 63, AlignCenter, AlignBottom, hit ? "HIT!" : "BLOCKED");
    }
}

void digiflip_tag_train_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyBack) {
        /* Walking away mid-session doesn't count. */
        app->screen = DigiflipScreenHome;
    } else if((input->key == InputKeyUp || input->key == InputKeyDown) && app->tag_shot_tick == 0) {
        app->tag_aim_high = input->key == InputKeyUp;
        app->tag_shot_tick = furi_get_tick();
        if(app->tag_aim_high != app->tag_guard_high)
            app->tag_hits |= (uint8_t)(1U << app->tag_shot);
        digiflip_sound_play(app->notification, DigiflipSoundHit);
    }
}

static uint8_t digiflip_count_bits(uint8_t bits) {
    uint8_t count = 0;
    for(; bits; bits >>= 1)
        count += bits & 1U;
    return count;
}

void digiflip_update_tag_train(DigiflipApp* app, uint32_t tick) {
    if(app->tag_shot_tick == 0 || tick - app->tag_shot_tick < DIGIFLIP_TAG_SHOT_MS) return;
    app->tag_shot_tick = 0;
    app->tag_shot++;
    if(app->tag_shot < DIGIFLIP_TAG_TRAIN_ROUNDS) {
        app->tag_guard_high = digiflip_game_tag_guard_high(app->game);
        return;
    }
    const uint8_t hits = digiflip_count_bits(app->tag_hits);
    const bool success = digiflip_game_tag_train(app->game, hits);
    /* A raised partner trains alongside. */
    DigiflipGame* mate = digiflip_mate_game(app);
    if(mate) digiflip_game_tag_train(mate, hits);
    digiflip_sound_play(app->notification, success ? DigiflipSoundSuccess : DigiflipSoundFailure);
    digiflip_show_notice(app, success ? "Great teamwork!" : "Not quite...");
    snprintf(
        app->notice_detail_buffer,
        sizeof(app->notice_detail_buffer),
        success ? "%u/5 hits! Strength up" : "%u/5 hits (need 3)",
        hits);
    app->notice_detail = app->notice_detail_buffer;
}

#define DIGIFLIP_SYNC_WIDTH 105
#define DIGIFLIP_SYNC_MARKS 4

static int16_t digiflip_sync_mark(uint8_t index) {
    return (int16_t)(DIGIFLIP_SYNC_WIDTH * (index + 1) / (DIGIFLIP_SYNC_MARKS + 1));
}

void digiflip_draw_tag_sync(Canvas* canvas, const DigiflipApp* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "AIM!");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, "Stop on a mark");
    digiflip_draw_character(canvas, 24, 12, digiflip_mate_species(app), DigiflipPoseIdle, true);
    digiflip_draw_character(canvas, 2, 12, app->game->species_id, DigiflipPoseIdle, true);
    const DigiflipTagRound* round = digiflip_game_next_tag_round(app->game);
    if(round) {
        digiflip_draw_character(
            canvas, 68, 12, digiflip_opponent_sprite_id(&round->right), DigiflipPoseIdle, false);
        digiflip_draw_character(
            canvas, 90, 12, digiflip_opponent_sprite_id(&round->left), DigiflipPoseIdle, false);
    }
    canvas_draw_frame(canvas, 10, 50, 110, 8);
    for(uint8_t i = 0; i < DIGIFLIP_SYNC_MARKS; i++) {
        const int16_t x = 12 + digiflip_sync_mark(i);
        canvas_draw_line(canvas, x, 48, x, 59);
    }
    canvas_draw_box(canvas, 11 + app->sync_cursor, 51, 3, 6);
}

void digiflip_update_tag_sync(DigiflipApp* app) {
    app->sync_cursor = (int16_t)(app->sync_cursor + app->sync_direction * 4);
    if(app->sync_cursor >= DIGIFLIP_SYNC_WIDTH) {
        app->sync_cursor = DIGIFLIP_SYNC_WIDTH;
        app->sync_direction = -1;
    } else if(app->sync_cursor <= 0) {
        app->sync_cursor = 0;
        app->sync_direction = 1;
    }
}

void digiflip_tag_sync_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk) {
        int16_t best = DIGIFLIP_SYNC_WIDTH;
        for(uint8_t i = 0; i < DIGIFLIP_SYNC_MARKS; i++) {
            int16_t distance = (int16_t)(app->sync_cursor + 1 - digiflip_sync_mark(i));
            if(distance < 0) distance = (int16_t)-distance;
            if(distance < best) best = distance;
        }
        app->battle_sync = best <= 2 ? DigiflipSyncPerfect :
                                       (best <= 6 ? DigiflipSyncNear : DigiflipSyncMiss);
        digiflip_sound_play(
            app->notification,
            app->battle_sync == DigiflipSyncMiss ? DigiflipSoundFailure : DigiflipSoundSuccess);
        digiflip_start_mash(app, DigiflipScreenBattleCharge);
    }
}

void digiflip_copymon_open(DigiflipApp* app) {
    app->slot2_confirm = false;
    app->copymon_index =
        digiflip_album_has(&app->album, app->copymon) ?
            app->copymon :
            digiflip_album_step(&app->album, (DigiflipSpeciesId)(digiflip_species_count - 1U), 1);
    app->screen = DigiflipScreenCopymon;
}

void digiflip_draw_copymon(Canvas* canvas, const DigiflipApp* app) {
    char buffer[32];
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "COPYMON");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_line(canvas, 0, 12, 127, 12);
    const DigiflipSpecies* species = digiflip_species_get(app->copymon_index);
    if(!species || !digiflip_album_has(&app->album, app->copymon_index)) {
        canvas_draw_str_aligned(
            canvas, 64, 30, AlignCenter, AlignCenter, "Raise a Digimon first:");
        canvas_draw_str_aligned(
            canvas, 64, 42, AlignCenter, AlignCenter, "any in your album can be one");
        return;
    }
    const bool current = app->slot2_kind == DigiflipSlotCopymon &&
                         app->copymon_index == app->copymon;
    canvas_draw_str_aligned(
        canvas, 126, 10, AlignRight, AlignBottom, current ? "Partner" : "< > browse");
    digiflip_draw_character(canvas, 4, 14, app->copymon_index, DigiflipPoseIdle, false);
    canvas_draw_str(canvas, 46, 23, digiflip_stage_name(species->stage));
    snprintf(buffer, sizeof(buffer), "Power %u", species->power + DIGIFLIP_STRENGTH_POWER_MAX);
    canvas_draw_str(canvas, 46, 34, buffer);
    canvas_draw_str(
        canvas,
        46,
        45,
        current            ? "In slot 2" :
        app->slot2_confirm ? "OK again: replace pet 2" :
                             "OK: put in slot 2");
    canvas_draw_line(canvas, 0, 52, 127, 52);
    canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, species->name);
}

void digiflip_copymon_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyLeft || input->key == InputKeyRight) {
        const DigiflipSpeciesId next = digiflip_album_step(
            &app->album, app->copymon_index, input->key == InputKeyLeft ? -1 : 1);
        if(next != DIGIFLIP_SPECIES_NONE) app->copymon_index = next;
        app->slot2_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyOk) {
        if(!digiflip_album_has(&app->album, app->copymon_index)) return;
        if(app->slot2_kind == DigiflipSlotPet && app->pets[1].alive && !app->slot2_confirm) {
            /* Replacing a living second pet takes a second OK. */
            app->slot2_confirm = true;
            digiflip_sound_play(app->notification, DigiflipSoundFailure);
            return;
        }
        app->slot2_confirm = false;
        digiflip_set_slot2(app, DigiflipSlotCopymon, app->copymon_index);
        if(app->view == DigiflipViewOne) digiflip_set_view(app, DigiflipViewBoth);
        digiflip_sound_play(app->notification, DigiflipSoundHappy);
    } else if(input->key == InputKeyBack) {
        app->slot2_confirm = false;
        app->screen = DigiflipScreenSlot;
    }
}
