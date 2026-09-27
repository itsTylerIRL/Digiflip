#include "digiflip_app.h"

void digiflip_apply_cheats(const DigiflipApp* app) {
    const DigiflipRules rules = {
        .freeze_hearts = app->settings.cheat_freeze_hearts,
        .no_waste = app->settings.cheat_no_waste,
    };
    digiflip_game_set_rules(&rules);
}

void digiflip_draw_cheats(Canvas* canvas, const DigiflipApp* app) {
    static const char* const labels[DigiflipCheatItemCount] = {
        "No heart loss", "No poop", "Fill hearts", "Force evolution", "All eggs", "Done"};
    const char* values[DigiflipCheatItemCount] = {
        app->settings.cheat_freeze_hearts ? "On" : "Off",
        app->settings.cheat_no_waste ? "On" : "Off",
        NULL,
        NULL,
        app->settings.cheat_all_eggs ? "On" : "Off",
        NULL,
    };
    digiflip_draw_list(
        canvas, "CHEATS", labels, values, DigiflipCheatItemCount, app->cheats_index);
}

void digiflip_cheats_open(DigiflipApp* app) {
    app->cheats_index = 0;
    app->screen = DigiflipScreenCheats;
}

static void digiflip_cheats_leave(DigiflipApp* app) {
    digiflip_settings_save(&app->settings);
    app->settings_index = DigiflipSettingCheats;
    app->screen = DigiflipScreenSettings;
}

static void digiflip_toggle_cheat(DigiflipApp* app, bool* flag, DigiflipCheat cheat) {
    *flag = !*flag;
    digiflip_apply_cheats(app);
    digiflip_log_app_event(app, DigiflipEventCheat, (uint8_t)cheat, *flag ? 1U : 0U);
    digiflip_sound_play(app->notification, DigiflipSoundClick);
}

void digiflip_cheats_input(DigiflipApp* app, const InputEvent* input) {
    const bool press = input->key == InputKeyOk || input->key == InputKeyLeft ||
                       input->key == InputKeyRight;
    if(input->key == InputKeyUp) {
        app->cheats_index = app->cheats_index == 0 ? DigiflipCheatItemCount - 1U :
                                                     app->cheats_index - 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        app->cheats_index = (app->cheats_index + 1U) % DigiflipCheatItemCount;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        digiflip_cheats_leave(app);
    } else if(press) {
        switch((DigiflipCheatItem)app->cheats_index) {
        case DigiflipCheatItemFreezeHearts:
            digiflip_toggle_cheat(
                app, &app->settings.cheat_freeze_hearts, DigiflipCheatFreezeHearts);
            break;
        case DigiflipCheatItemNoWaste:
            digiflip_toggle_cheat(app, &app->settings.cheat_no_waste, DigiflipCheatNoWaste);
            break;
        case DigiflipCheatItemFillHearts:
            if(input->key != InputKeyOk) break;
            if(!app->game->alive || app->game->stage == DigiflipStageEgg) {
                digiflip_refuse(app, "Nothing to fill yet");
                app->notice_return = DigiflipScreenCheats;
                break;
            }
            digiflip_game_fill_hearts(app->game);
            digiflip_log_app_event(app, DigiflipEventCheat, DigiflipCheatFillHearts, 0);
            digiflip_sound_play(app->notification, DigiflipSoundHappy);
            digiflip_show_notice(app, "Hearts filled!");
            app->notice_return = DigiflipScreenCheats;
            break;
        case DigiflipCheatItemForceEvolution:
            if(input->key != InputKeyOk) break;
            digiflip_log_app_event(app, DigiflipEventCheat, DigiflipCheatForceEvolution, 0);
            if(digiflip_game_force_evolve(app->game)) {
                /* Head home so the evolution animation plays. */
                digiflip_settings_save(&app->settings);
                app->views[0].reaction = DigiflipReactionNone;
                app->views[1].reaction = DigiflipReactionNone;
                app->clean_start_tick = 0;
                app->screen = DigiflipScreenHome;
            } else {
                digiflip_refuse(app, app->game->alive ? "Can't evolve further" : "No pet");
                app->notice_return = DigiflipScreenCheats;
            }
            break;
        case DigiflipCheatItemAllEggs:
            digiflip_toggle_cheat(app, &app->settings.cheat_all_eggs, DigiflipCheatAllEggs);
            break;
        case DigiflipCheatItemDone:
            if(input->key == InputKeyOk) digiflip_cheats_leave(app);
            break;
        case DigiflipCheatItemCount:
            break;
        }
    }
}
