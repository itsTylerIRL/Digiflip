#include "digiflip_app.h"

#define DIGIFLIP_LOG_ROWS 4U

void digiflip_draw_logs(Canvas* canvas, const DigiflipApp* app) {
    char buffer[48];
    char when[24];
    const uint16_t count = app->log.count;

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "LOGS");
    canvas_set_font(canvas, FontSecondary);
    if(count == 0) {
        canvas_draw_line(canvas, 0, 12, 127, 12);
        canvas_draw_str_aligned(canvas, 64, 36, AlignCenter, AlignCenter, "No events yet");
        return;
    }
    snprintf(buffer, sizeof(buffer), "%u/%u", app->log_index + 1U, count);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, buffer);
    canvas_draw_line(canvas, 0, 12, 127, 12);

    uint16_t first = app->log_index > 0 ? app->log_index - 1U : 0U;
    if(count > DIGIFLIP_LOG_ROWS && first > count - DIGIFLIP_LOG_ROWS) {
        first = count - DIGIFLIP_LOG_ROWS;
    }
    for(uint16_t row = 0; row < DIGIFLIP_LOG_ROWS && first + row < count; row++) {
        const uint16_t index = first + row;
        const DigiflipEvent* event = digiflip_log_get(&app->log, index);
        if(!event) break;
        const int16_t y = 14 + row * 10;
        if(index == app->log_index) {
            canvas_draw_rbox(canvas, 0, y, 128, 10, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        digiflip_time_short(event->time, when, sizeof(when));
        canvas_draw_str(canvas, 2, y + 8, when + 6);
        const size_t tag = event->slot == 1 ? 2U : 0U;
        if(tag) memcpy(buffer, "2:", 2);
        digiflip_event_describe(event, buffer + tag, sizeof(buffer) - tag);
        canvas_draw_str(canvas, 29, y + 8, buffer);
        canvas_set_color(canvas, ColorBlack);
    }

    const DigiflipEvent* selected = digiflip_log_get(&app->log, app->log_index);
    canvas_draw_line(canvas, 0, 54, 127, 54);
    if(selected) {
        digiflip_time_long(selected->time, when, sizeof(when));
        canvas_draw_str_aligned(canvas, 2, 63, AlignLeft, AlignBottom, when);
    }
    canvas_draw_str_aligned(canvas, 126, 63, AlignRight, AlignBottom, "OK:save");
}

void digiflip_logs_open(DigiflipApp* app) {
    app->log_index = 0;
    app->screen = DigiflipScreenLogs;
}

void digiflip_logs_input(DigiflipApp* app, const InputEvent* input) {
    const uint16_t count = app->log.count;
    const uint16_t last = count ? count - 1U : 0U;
    if(input->key == InputKeyUp) {
        if(app->log_index > 0) app->log_index--;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyDown) {
        if(app->log_index < last) app->log_index++;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyLeft) {
        app->log_index = app->log_index > DIGIFLIP_LOG_ROWS ? app->log_index - DIGIFLIP_LOG_ROWS :
                                                              0U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyRight) {
        app->log_index =
            app->log_index + DIGIFLIP_LOG_ROWS < last ? app->log_index + DIGIFLIP_LOG_ROWS : last;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyOk && count > 0) {
        const bool saved = digiflip_log_export(&app->log);
        digiflip_sound_play(
            app->notification, saved ? DigiflipSoundSuccess : DigiflipSoundFailure);
        digiflip_show_notice(app, saved ? "Saved log.txt" : "Couldn't save log");
        app->notice_detail = saved ? "apps_data/digiflip" : NULL;
        app->notice_return = DigiflipScreenLogs;
    } else if(input->key == InputKeyBack) {
        app->settings_index = DigiflipSettingLogs;
        app->screen = DigiflipScreenSettings;
    }
}

void digiflip_log_event(void* context, const DigiflipGame* game, const DigiflipEvent* event) {
    DigiflipApp* app = context;
    DigiflipEvent tagged = *event;
    tagged.slot = game == &app->pets[1] ? 1U : 0U;
    digiflip_log_push(&app->log, &tagged);
    app->log_dirty = true;
    /* Keep the selection on the same event when new ones arrive on screen. */
    if(app->screen == DigiflipScreenLogs && app->log_index > 0 &&
       app->log_index + 1U < app->log.count) {
        app->log_index++;
    }
}

void digiflip_log_app_event(DigiflipApp* app, DigiflipEventType type, uint8_t a, uint16_t b) {
    const DigiflipEvent event = {
        .time = furi_hal_rtc_get_timestamp(),
        .type = (uint8_t)type,
        .a = a,
        .b = b,
    };
    digiflip_log_event(app, app->game, &event);
}
