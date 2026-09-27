#include "digiflip_app.h"

static void digiflip_draw_callback(Canvas* canvas, void* context) {
    DigiflipApp* app = context;
    furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    /* Sprites draw only their ink, so overlaps don't erase what's behind. */
    canvas_set_bitmap_mode(canvas, true);

    switch(app->screen) {
    case DigiflipScreenEggSelect:
        digiflip_draw_egg_select(canvas, app);
        break;
    case DigiflipScreenHome:
        digiflip_draw_home(canvas, app);
        break;
    case DigiflipScreenStatus:
        digiflip_draw_status(canvas, app);
        break;
    case DigiflipScreenFeed:
        digiflip_draw_feed(canvas, app);
        break;
    case DigiflipScreenTrain:
        digiflip_draw_train(canvas, app);
        break;
    case DigiflipScreenBattleIntro:
        digiflip_draw_battle_intro(canvas, app);
        break;
    case DigiflipScreenBattleCharge:
        digiflip_draw_battle_charge(canvas, app);
        break;
    case DigiflipScreenBattle:
        digiflip_draw_battle(canvas, app);
        break;
    case DigiflipScreenNotice:
        digiflip_draw_notice(canvas, app);
        break;
    case DigiflipScreenDeath:
        digiflip_draw_death(canvas, app);
        break;
    case DigiflipScreenPause:
        digiflip_draw_pause(canvas, app);
        break;
    case DigiflipScreenSettings:
        digiflip_draw_settings(canvas, app);
        break;
    case DigiflipScreenAlbum:
        digiflip_draw_album(canvas, app);
        break;
    case DigiflipScreenLogs:
        digiflip_draw_logs(canvas, app);
        break;
    case DigiflipScreenCheats:
        digiflip_draw_cheats(canvas, app);
        break;
    case DigiflipScreenModePick:
        digiflip_draw_mode_pick(canvas, app);
        break;
    case DigiflipScreenTagTrain:
        digiflip_draw_tag_train(canvas, app);
        break;
    case DigiflipScreenTagSync:
        digiflip_draw_tag_sync(canvas, app);
        break;
    case DigiflipScreenCopymon:
        digiflip_draw_copymon(canvas, app);
        break;
    case DigiflipScreenSlot:
        digiflip_draw_slot_menu(canvas, app);
        break;
    }
    canvas_set_bitmap_mode(canvas, false);
    furi_mutex_release(app->mutex);
}

static void digiflip_input_callback(InputEvent* input, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, input, 0);
}

static void digiflip_handle_input(DigiflipApp* app, const InputEvent* input) {
    /* Mash screens count raw presses; everything else acts on short presses. */
    if(app->screen == DigiflipScreenTrain || app->screen == DigiflipScreenBattleCharge) {
        digiflip_handle_mash(app, input);
        return;
    }
    /* Holding OK on the home screen flips between views 1, 1&2 and 2. */
    if(app->screen == DigiflipScreenHome && input->key == InputKeyOk &&
       input->type == InputTypeLong) {
        if(!digiflip_home_busy(app) && app->slot2_kind != DigiflipSlotEmpty) {
            digiflip_cycle_view(app);
            digiflip_sound_play(app->notification, DigiflipSoundClick);
        }
        return;
    }
    if(input->type != InputTypeShort && input->type != InputTypeRepeat) return;

    switch(app->screen) {
    case DigiflipScreenEggSelect:
        digiflip_egg_select_input(app, input);
        break;
    case DigiflipScreenHome:
        digiflip_handle_home(app, input);
        break;
    case DigiflipScreenStatus:
        digiflip_status_input(app, input);
        break;
    case DigiflipScreenFeed:
        digiflip_feed_input(app, input);
        break;
    case DigiflipScreenBattleIntro:
        digiflip_battle_intro_input(app, input);
        break;
    case DigiflipScreenBattle:
        digiflip_battle_input(app, input);
        break;
    case DigiflipScreenNotice:
        digiflip_notice_input(app, input);
        break;
    case DigiflipScreenPause:
        digiflip_handle_pause(app, input);
        break;
    case DigiflipScreenSettings:
        digiflip_handle_settings(app, input);
        break;
    case DigiflipScreenAlbum:
        digiflip_album_input(app, input);
        break;
    case DigiflipScreenLogs:
        digiflip_logs_input(app, input);
        break;
    case DigiflipScreenCheats:
        digiflip_cheats_input(app, input);
        break;
    case DigiflipScreenModePick:
        digiflip_mode_input(app, input);
        break;
    case DigiflipScreenTagTrain:
        digiflip_tag_train_input(app, input);
        break;
    case DigiflipScreenTagSync:
        digiflip_tag_sync_input(app, input);
        break;
    case DigiflipScreenCopymon:
        digiflip_copymon_input(app, input);
        break;
    case DigiflipScreenSlot:
        digiflip_slot_menu_input(app, input);
        break;
    case DigiflipScreenDeath:
        digiflip_death_input(app, input);
        break;
    case DigiflipScreenTrain:
    case DigiflipScreenBattleCharge:
        break;
    }
}

/* Load whoever is about to be drawn into the sprite cache. Cheap when they're
   already cached; runs on the main thread so drawing never reads the SD card. */
static void digiflip_prepare_sprites(const DigiflipApp* app) {
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_visible(app, slot)) continue;
        if(!digiflip_slot_raised(app, slot)) {
            digiflip_sprites_prepare(app->copymon);
            continue;
        }
        const DigiflipGame* game = &app->pets[slot];
        if(game->stage != DigiflipStageEgg) digiflip_sprites_prepare(game->species_id);
        if(digiflip_game_evo_pending(game)) {
            digiflip_sprites_prepare(digiflip_game_evo_from_species(game));
        }
    }
    if(app->game->stage != DigiflipStageEgg) digiflip_sprites_prepare(app->game->species_id);
    switch(app->screen) {
    case DigiflipScreenBattleIntro:
    case DigiflipScreenBattleCharge:
    case DigiflipScreenTagSync:
        if(app->battle_tag || app->screen == DigiflipScreenTagSync) {
            const DigiflipTagRound* round = digiflip_game_next_tag_round(app->game);
            digiflip_sprites_prepare(digiflip_mate_species(app));
            if(round) {
                digiflip_sprites_prepare(digiflip_opponent_sprite_id(&round->left));
                digiflip_sprites_prepare(digiflip_opponent_sprite_id(&round->right));
            }
        } else {
            digiflip_sprites_prepare(
                digiflip_opponent_sprite_id(digiflip_game_next_opponent(app->game)));
        }
        break;
    case DigiflipScreenBattle:
        digiflip_sprites_prepare(app->battle.player_species);
        digiflip_sprites_prepare(digiflip_opponent_sprite_id(app->battle.opponent));
        if(app->battle.tag) {
            digiflip_sprites_prepare(app->battle.partner_species);
            digiflip_sprites_prepare(digiflip_opponent_sprite_id(app->battle.opponent2));
        }
        break;
    case DigiflipScreenTagTrain:
        digiflip_sprites_prepare(digiflip_mate_species(app));
        break;
    case DigiflipScreenCopymon:
        if(digiflip_album_has(&app->album, app->copymon_index)) {
            digiflip_sprites_prepare(app->copymon_index);
        }
        break;
    case DigiflipScreenAlbum:
        if(digiflip_album_has(&app->album, app->album_index)) {
            digiflip_sprites_prepare(app->album_index);
        }
        break;
    default:
        break;
    }
}

static void digiflip_save_log(DigiflipApp* app) {
    if(app->log_dirty && digiflip_log_save(&app->log)) app->log_dirty = false;
}

/* Highest-priority call among the raised pets. */
static uint8_t digiflip_call(const DigiflipApp* app) {
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_raised(app, slot)) continue;
        const uint8_t call = (uint8_t)digiflip_game_call_reason(&app->pets[slot]);
        if(call != DigiflipCallNone) return call;
    }
    return DigiflipCallNone;
}

static void digiflip_save_pets(const DigiflipApp* app) {
    digiflip_storage_save_slot(&app->pets[0], 0);
    if(app->slot2_kind == DigiflipSlotPet) digiflip_storage_save_slot(&app->pets[1], 1);
}

/* Evolution animation for the first visible pet with one pending. Its timer
   runs only on the home screen so it always plays fully while watched. */
static void digiflip_update_evo(DigiflipApp* app, uint32_t tick) {
    if(app->evo_anim_start_tick != 0) {
        DigiflipGame* game = &app->pets[app->evo_slot];
        if(!digiflip_game_evo_pending(game) || !digiflip_slot_visible(app, app->evo_slot)) {
            app->evo_anim_start_tick = 0;
            app->evo_album_new = false;
        } else if(tick - app->evo_anim_start_tick >= DIGIFLIP_EVO_ANIM_MS) {
            digiflip_game_clear_evo_pending(game);
            app->evo_anim_start_tick = 0;
            app->evo_album_new = false;
        }
        return;
    }
    if(app->screen != DigiflipScreenHome || app->views[0].reaction != DigiflipReactionNone ||
       app->views[1].reaction != DigiflipReactionNone || app->clean_start_tick != 0)
        return;
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(digiflip_slot_visible(app, slot) && digiflip_slot_raised(app, slot) &&
           digiflip_game_evo_pending(&app->pets[slot])) {
            app->evo_slot = slot;
            app->evo_anim_start_tick = tick;
            return;
        }
    }
}

static void digiflip_update_frame(DigiflipApp* app, uint32_t tick) {
    const uint32_t now = furi_hal_rtc_get_timestamp();
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_raised(app, slot)) continue;
        DigiflipGame* game = &app->pets[slot];
        const uint8_t previous_stage = game->stage;
        const bool was_alive = game->alive;
        digiflip_game_tick(game, now);
        if(digiflip_album_note(app, slot)) {
            if(digiflip_game_evo_pending(game)) app->evo_album_new = true;
            digiflip_check_unlocks(app);
        }
        if(game->stage != previous_stage) {
            digiflip_sound_play(app->notification, DigiflipSoundHatch);
        }
        if(!game->alive && app->screen != DigiflipScreenDeath &&
           app->screen != DigiflipScreenEggSelect) {
            if(was_alive) digiflip_sound_play(app->notification, DigiflipSoundDeath);
            digiflip_show_death(app, slot);
        }
    }

    digiflip_prepare_sprites(app);

    const uint8_t call = digiflip_call(app);
    if(call != DigiflipCallNone && call != app->last_call_reason) {
        digiflip_sound_play(app->notification, DigiflipSoundCall);
    }
    app->last_call_reason = call;
    const bool want_led = call != DigiflipCallNone && app->settings.call_light;
    if(want_led != app->call_led_on) {
        app->call_led_on = want_led;
        digiflip_call_led(app->notification, want_led);
    }

    digiflip_update_evo(app, tick);

    if(app->unlock_notice && app->screen == DigiflipScreenHome && !digiflip_home_busy(app)) {
        const uint8_t egg = (uint8_t)(app->unlock_notice - 1U);
        app->unlock_notice = 0;
        digiflip_sound_play(app->notification, DigiflipSoundSuccess);
        digiflip_show_notice(app, "New Digi-Egg!");
        app->notice_detail = digiflip_eggs[egg].name;
    }

    if(tick - app->last_save_tick >= 60000U) {
        app->last_save_tick = tick;
        digiflip_save_pets(app);
        digiflip_save_log(app);
    }

    switch(app->screen) {
    case DigiflipScreenTrain:
        if(app->mash_start_tick && !digiflip_mash_remaining(app, DIGIFLIP_TRAIN_WINDOW_MS)) {
            digiflip_finish_training(app);
        }
        break;
    case DigiflipScreenBattleCharge:
        if(app->mash_start_tick && !digiflip_mash_remaining(app, DIGIFLIP_BATTLE_CHARGE_MS)) {
            digiflip_start_battle(app);
        }
        break;
    case DigiflipScreenBattle:
        digiflip_update_battle(app, tick);
        break;
    case DigiflipScreenTagTrain:
        digiflip_update_tag_train(app, tick);
        break;
    case DigiflipScreenTagSync:
        digiflip_update_tag_sync(app);
        break;
    case DigiflipScreenHome:
        /* Back home, the focus follows the view again (training or a battle
           may have made the other pet the lead). */
        digiflip_refocus(app);
        digiflip_update_pet_walk(app, tick);
        break;
    default:
        break;
    }
}

int32_t digiflip_app(void* p) {
    UNUSED(p);
    DigiflipApp* app = malloc(sizeof(DigiflipApp));
    furi_check(app);
    *app = (DigiflipApp){
        .mutex = furi_mutex_alloc(FuriMutexTypeNormal),
        .queue = furi_message_queue_alloc(16, sizeof(InputEvent)),
        .notification = furi_record_open(RECORD_NOTIFICATION),
        .screen = DigiflipScreenHome,
        .views = {{.x = 46, .direction = 1}, {.x = 46, .direction = 1}},
        .last_frame_tick = furi_get_tick(),
        .last_pet_step_tick = furi_get_tick(),
        .last_save_tick = furi_get_tick(),
        .album_last_species = {DIGIFLIP_SPECIES_NONE, DIGIFLIP_SPECIES_NONE},
        .album_owned_only = true,
        .copymon = DIGIFLIP_SPECIES_NONE,
        .running = true,
    };

    digiflip_settings_load(&app->settings);
    digiflip_apply_settings(app);
    /* Cheats and the log must be in place before the first tick replays the
       time the app was closed. */
    digiflip_apply_cheats(app);
    digiflip_log_load(&app->log);
    app->game = &app->pets[0];

    const uint32_t now = furi_hal_rtc_get_timestamp();
    if(!digiflip_storage_load_slot(&app->pets[0], 0)) {
        digiflip_game_new(&app->pets[0], now);
        app->screen = DigiflipScreenEggSelect;
    }
    digiflip_slot2_load(&app->slot2_kind, &app->copymon);
    if(app->slot2_kind == DigiflipSlotPet && !digiflip_storage_load_slot(&app->pets[1], 1)) {
        app->slot2_kind = DigiflipSlotEmpty;
    }
    if(app->slot2_kind == DigiflipSlotCopymon && !digiflip_species_get(app->copymon)) {
        app->slot2_kind = DigiflipSlotEmpty;
    }
    if(app->slot2_kind != DigiflipSlotCopymon) app->copymon = DIGIFLIP_SPECIES_NONE;
    digiflip_set_view(
        app, app->slot2_kind == DigiflipSlotEmpty ? DigiflipViewOne : DigiflipViewBoth);
    digiflip_game_set_event_sink(digiflip_log_event, app);
    /* Existing pets join the album the first time they're seen. */
    digiflip_album_load(&app->album);
    if(!digiflip_records_load(&app->records)) {
        /* First run with records: start from the pets' own wins, and treat
           what's already unlocked as announced. */
        for(uint8_t slot = 0; slot < 2; slot++) {
            if(digiflip_slot_raised(app, slot))
                app->records.wins += app->pets[slot].total_victories;
        }
        app->records.known_eggs = digiflip_eggs_unlocked(&app->album, app->records.wins);
        digiflip_records_save(&app->records);
    }
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(!digiflip_slot_raised(app, slot)) continue;
        DigiflipGame* game = &app->pets[slot];
        const uint8_t starting_stage = game->stage;
        digiflip_game_tick(game, now);
        if(game->stage != starting_stage) {
            digiflip_sound_play(app->notification, DigiflipSoundHatch);
        }
        if(digiflip_album_note(app, slot)) {
            if(digiflip_game_evo_pending(game)) app->evo_album_new = true;
            digiflip_check_unlocks(app);
        }
        if(!game->alive && app->screen != DigiflipScreenDeath &&
           app->screen != DigiflipScreenEggSelect) {
            digiflip_show_death(app, slot);
        }
    }

    digiflip_sprites_init();
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, digiflip_draw_callback, app);
    view_port_input_callback_set(view_port, digiflip_input_callback, app->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    while(app->running) {
        InputEvent input;
        const bool got_input = furi_message_queue_get(app->queue, &input, 50U) == FuriStatusOk;

        furi_check(furi_mutex_acquire(app->mutex, FuriWaitForever) == FuriStatusOk);
        if(got_input) digiflip_handle_input(app, &input);
        const uint32_t tick = furi_get_tick();
        const bool frame_due = tick - app->last_frame_tick >= 50U;
        if(frame_due) {
            app->last_frame_tick = tick;
            digiflip_update_frame(app, tick);
        }
        furi_mutex_release(app->mutex);

        if(frame_due || got_input) view_port_update(view_port);
    }

    digiflip_save_pets(app);
    digiflip_save_log(app);
    digiflip_game_set_event_sink(NULL, NULL);
    if(app->call_led_on) digiflip_call_led(app->notification, false);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    view_port_free(view_port);
    digiflip_sprites_free();
    furi_message_queue_free(app->queue);
    furi_mutex_free(app->mutex);
    free(app);
    return 0;
}
