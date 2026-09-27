#include "digiflip_app.h"

void digiflip_play_click(DigiflipApp* app, const InputEvent* input) {
    if(input->type == InputTypeShort) {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
    }
}

void digiflip_show_notice(DigiflipApp* app, const char* notice) {
    app->notice = notice;
    app->notice_detail = NULL;
    app->notice_return = DigiflipScreenHome;
    app->screen = DigiflipScreenNotice;
}

void digiflip_refuse(DigiflipApp* app, const char* notice) {
    digiflip_sound_play(app->notification, DigiflipSoundFailure);
    digiflip_show_notice(app, notice);
}

void digiflip_draw_notice(Canvas* canvas, const DigiflipApp* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 64, app->notice_detail ? 22 : 28, AlignCenter, AlignCenter, app->notice);
    canvas_set_font(canvas, FontSecondary);
    if(app->notice_detail) {
        canvas_draw_str_aligned(canvas, 64, 36, AlignCenter, AlignCenter, app->notice_detail);
    }
    canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignCenter, "OK / BACK");
}

void digiflip_show_death(DigiflipApp* app, uint8_t slot) {
    const DigiflipGame* game = &app->pets[slot];
    app->death_slot = slot;
    snprintf(
        app->notice_buffer, sizeof(app->notice_buffer), "%s", digiflip_game_species_name(game));
    snprintf(
        app->notice_detail_buffer,
        sizeof(app->notice_detail_buffer),
        "Cause: %s",
        digiflip_game_death_cause_name(digiflip_game_death_cause(game)));
    app->views[0].reaction = DigiflipReactionNone;
    app->views[1].reaction = DigiflipReactionNone;
    app->evo_anim_start_tick = 0;
    app->clean_start_tick = 0;
    app->screen = DigiflipScreenDeath;
}

void digiflip_draw_death(Canvas* canvas, const DigiflipApp* app) {
    canvas_draw_rframe(canvas, 50, 4, 28, 28, 7);
    canvas_draw_line(canvas, 64, 9, 64, 24);
    canvas_draw_line(canvas, 58, 14, 70, 14);
    canvas_draw_line(canvas, 36, 32, 92, 32);
    canvas_draw_line(canvas, 42, 31, 86, 31);
    const int16_t drift = (int16_t)((furi_get_tick() / 300U) % 6U);
    canvas_draw_circle(canvas, 88, 8 - drift / 2, 3);
    canvas_draw_line(canvas, 86, 11 - drift / 2, 85, 14 - drift / 2);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 44, AlignCenter, AlignBottom, app->notice_buffer);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 53, AlignCenter, AlignBottom, app->notice_detail_buffer);
    canvas_draw_str_aligned(
        canvas,
        64,
        63,
        AlignCenter,
        AlignBottom,
        app->pets[app->death_slot].trait_earned ?
            (app->death_slot ? "OK traited egg  BACK empty" : "Traited egg! OK: new egg") :
            (app->death_slot ? "OK new egg  BACK empty slot" : "OK: new Digi-Egg"));
}

void digiflip_notice_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyOk || input->key == InputKeyBack) app->screen = app->notice_return;
}

void digiflip_death_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyBack && app->death_slot == 1) {
        digiflip_set_slot2(app, DigiflipSlotEmpty, DIGIFLIP_SPECIES_NONE);
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk || input->key == InputKeyBack) {
        app->egg_slot = app->death_slot;
        app->egg_confirm = false;
        app->selected_egg = app->pets[app->death_slot].egg_id;
        app->egg_select_from_home = false;
        app->screen = DigiflipScreenEggSelect;
    }
}
