#include "digiflip_app.h"

void digiflip_start_mash(DigiflipApp* app, DigiflipScreen screen) {
    app->mash_start_tick = 0;
    app->mash_presses = 0;
    app->screen = screen;
}

/* Mash screens count raw presses so fast tapping registers every hit. */
void digiflip_handle_mash(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyOk && input->type == InputTypePress) {
        if(app->mash_start_tick == 0) app->mash_start_tick = furi_get_tick();
        if(app->mash_presses < UINT16_MAX) app->mash_presses++;
        digiflip_sound_play(app->notification, DigiflipSoundHit);
    } else if(input->key == InputKeyBack && input->type == InputTypeShort) {
        /* Backing out before the timer ends costs nothing. */
        app->screen = DigiflipScreenHome;
    }
}

void digiflip_draw_train(Canvas* canvas, const DigiflipApp* app) {
    char buffer[16];
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "TRAIN");
    canvas_set_font(canvas, FontSecondary);
    snprintf(buffer, sizeof(buffer), "%u/%u", app->mash_presses, DIGIFLIP_TRAIN_PRESSES_NEEDED);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, buffer);

    const bool punch = (app->mash_presses & 1U) != 0;
    digiflip_draw_character(
        canvas,
        punch ? 34 : 30,
        11,
        app->game->species_id,
        punch ? DigiflipPoseAttack : DigiflipPoseIdle,
        true);
    const int16_t bag_x = punch ? 88 : 84;
    canvas_draw_line(canvas, 84, 11, bag_x + 5, 17);
    canvas_draw_rframe(canvas, bag_x, 17, 11, 24, 3);
    canvas_draw_line(canvas, bag_x + 2, 24, bag_x + 8, 24);
    canvas_draw_line(canvas, bag_x + 2, 33, bag_x + 8, 33);

    if(app->mash_start_tick == 0 && ((furi_get_tick() / 400U) & 1U) == 0) {
        canvas_draw_str_aligned(canvas, 64, 20, AlignCenter, AlignBottom, "Mash OK!");
    }
    digiflip_draw_meter(
        canvas,
        50,
        app->mash_presses,
        DIGIFLIP_TRAIN_PRESSES_NEEDED + 4U,
        DIGIFLIP_TRAIN_PRESSES_NEEDED);
    digiflip_draw_mash_timer(canvas, app, DIGIFLIP_TRAIN_WINDOW_MS);
}

void digiflip_finish_training(DigiflipApp* app) {
    const uint16_t presses = app->mash_presses;
    const bool success = digiflip_game_train(app->game, presses);
    digiflip_sound_play(app->notification, success ? DigiflipSoundSuccess : DigiflipSoundFailure);
    digiflip_show_notice(app, success ? "Great training!" : "Not enough...");
    snprintf(
        app->notice_detail_buffer,
        sizeof(app->notice_detail_buffer),
        success ? "%u hits! Strength up" : "%u hits (need %u)",
        presses,
        DIGIFLIP_TRAIN_PRESSES_NEEDED);
    app->notice_detail = app->notice_detail_buffer;
}
