#include "digiflip_app.h"

void digiflip_draw_pause(Canvas* canvas, const DigiflipApp* app) {
    static const char* const labels[DigiflipPauseCount] = {
        "Resume", "View", "Slot 1", "Slot 2", "Settings", "Album", "Save & Exit"};
    const char* values[DigiflipPauseCount] = {NULL};
    values[DigiflipPauseView] = digiflip_view_name(app->view);
    char slot1[24];
    char slot2[24];
    snprintf(slot1, sizeof(slot1), "%.14s", digiflip_slot_label(app, 0));
    if(app->slot2_kind == DigiflipSlotCopymon) {
        snprintf(slot2, sizeof(slot2), "%.9s copy", digiflip_slot_label(app, 1));
    } else {
        snprintf(slot2, sizeof(slot2), "%.14s", digiflip_slot_label(app, 1));
    }
    values[DigiflipPauseSlot1] = slot1;
    values[DigiflipPauseSlot2] = slot2;
    digiflip_draw_list(canvas, "MENU", labels, values, DigiflipPauseCount, app->pause_index);
}

void digiflip_draw_settings(Canvas* canvas, const DigiflipApp* app) {
    static const char* const labels[DigiflipSettingCount] = {
        "Sound", "Call light", "Logs", "Cheats", "Done"};
    const char* values[DigiflipSettingCount] = {
        app->settings.sound ? "On" : "Off",
        app->settings.call_light ? "On" : "Off",
        NULL,
        NULL,
        NULL,
    };
    digiflip_draw_list(
        canvas, "SETTINGS", labels, values, DigiflipSettingCount, app->settings_index);
}

void digiflip_handle_pause(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyUp) {
        app->pause_index = app->pause_index == 0 ? DigiflipPauseCount - 1U : app->pause_index - 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        app->pause_index = (app->pause_index + 1U) % DigiflipPauseCount;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk) {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
        switch((DigiflipPauseItem)app->pause_index) {
        case DigiflipPauseResume:
            app->screen = DigiflipScreenHome;
            break;
        case DigiflipPauseAlbum:
            digiflip_album_open(app);
            break;
        case DigiflipPauseView:
            if(app->slot2_kind == DigiflipSlotEmpty) {
                digiflip_refuse(app, "Slot 2 is empty");
                app->notice_return = DigiflipScreenPause;
            } else {
                digiflip_cycle_view(app);
            }
            break;
        case DigiflipPauseSlot1:
        case DigiflipPauseSlot2:
            digiflip_slot_open(app, app->pause_index == DigiflipPauseSlot1 ? 0U : 1U);
            break;
        case DigiflipPauseSettings:
            app->settings_index = DigiflipSettingSound;
            app->screen = DigiflipScreenSettings;
            break;
        case DigiflipPauseExit:
        case DigiflipPauseCount:
            app->running = false;
            break;
        }
    }
}

void digiflip_apply_settings(DigiflipApp* app) {
    digiflip_sound_set_enabled(app->settings.sound);
    /* Re-evaluated on the next frame against the current call. */
    if(app->call_led_on && !app->settings.call_light) {
        app->call_led_on = false;
        digiflip_call_led(app->notification, false);
    }
}

void digiflip_handle_settings(DigiflipApp* app, const InputEvent* input) {
    const bool toggle = input->key == InputKeyOk || input->key == InputKeyLeft ||
                        input->key == InputKeyRight;
    if(input->key == InputKeyUp) {
        app->settings_index = app->settings_index == 0 ? DigiflipSettingCount - 1U :
                                                         app->settings_index - 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        app->settings_index = (app->settings_index + 1U) % DigiflipSettingCount;
        digiflip_play_click(app, input);
    } else if(
        input->key == InputKeyBack ||
        (input->key == InputKeyOk && app->settings_index == DigiflipSettingDone)) {
        digiflip_settings_save(&app->settings);
        app->screen = DigiflipScreenPause;
    } else if(toggle && app->settings_index == DigiflipSettingSound) {
        app->settings.sound = !app->settings.sound;
        digiflip_apply_settings(app);
        digiflip_sound_play(app->notification, DigiflipSoundClick);
    } else if(toggle && app->settings_index == DigiflipSettingCallLight) {
        app->settings.call_light = !app->settings.call_light;
        digiflip_apply_settings(app);
        digiflip_sound_play(app->notification, DigiflipSoundClick);
    } else if(input->key == InputKeyOk && app->settings_index == DigiflipSettingLogs) {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
        digiflip_logs_open(app);
    } else if(input->key == InputKeyOk && app->settings_index == DigiflipSettingCheats) {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
        digiflip_cheats_open(app);
    }
}
