#include "digiflip_app.h"

void digiflip_draw_status(Canvas* canvas, const DigiflipApp* app) {
    char buffer[48];
    char title[20];
    const DigiflipSpecies* species = digiflip_species_get(app->game->species_id);
    canvas_set_font(canvas, FontSecondary);
    snprintf(title, sizeof(title), "%.18s", digiflip_game_species_name(app->game));
    canvas_draw_str(canvas, 2, 8, title);
    uint8_t targets[2];
    if(digiflip_care_targets(app, targets) == 2) {
        snprintf(
            buffer,
            sizeof(buffer),
            "P%u  %u/4",
            digiflip_focus_slot(app) + 1U,
            app->status_page + 1U);
    } else {
        snprintf(buffer, sizeof(buffer), "%u/4", app->status_page + 1U);
    }
    canvas_draw_str_aligned(canvas, 126, 8, AlignRight, AlignBottom, buffer);
    canvas_draw_line(canvas, 0, 10, 127, 10);
    digiflip_draw_pet(canvas, app, 3, 14, DigiflipPoseIdle, false);

    if(app->status_page == 0) {
        canvas_draw_str(canvas, 44, 20, "Hunger");
        digiflip_draw_hearts(canvas, 90, 14, app->game->hunger);
        canvas_draw_str(canvas, 44, 32, "Strength");
        digiflip_draw_hearts(canvas, 90, 26, app->game->strength);
        canvas_draw_str(canvas, 44, 44, "Effort");
        digiflip_draw_hearts(canvas, 90, 38, app->game->effort);
        snprintf(
            buffer,
            sizeof(buffer),
            "Age %u days    Weight %uG",
            digiflip_game_age_days(app->game, furi_hal_rtc_get_timestamp()),
            app->game->weight);
    } else if(app->status_page == 1) {
        canvas_draw_str(canvas, 44, 20, digiflip_game_stage_name(app->game));
        snprintf(buffer, sizeof(buffer), "Care mistakes  %u", app->game->care_mistakes);
        canvas_draw_str(canvas, 44, 31, buffer);
        snprintf(buffer, sizeof(buffer), "Training       %u", app->game->trainings);
        canvas_draw_str(canvas, 44, 42, buffer);
        snprintf(buffer, sizeof(buffer), "Overfeeds      %u", app->game->overfeeds);
        canvas_draw_str(canvas, 44, 53, buffer);
        const uint32_t duration = digiflip_stage_duration((DigiflipStage)app->game->stage);
        if(duration == 0) {
            snprintf(buffer, sizeof(buffer), "Next: tag evolution only");
        } else if(app->game->evolution_attempted) {
            snprintf(buffer, sizeof(buffer), "Final form reached");
        } else {
            const uint32_t remaining =
                app->game->stage_elapsed < duration ? duration - app->game->stage_elapsed : 0;
            snprintf(
                buffer,
                sizeof(buffer),
                "Next: %luh %02lum",
                (unsigned long)(remaining / 3600U),
                (unsigned long)((remaining / 60U) % 60U));
        }
        if(app->game->traited) {
            const size_t used = strlen(buffer);
            snprintf(buffer + used, sizeof(buffer) - used, "  Traited");
        }
    } else if(app->status_page == 2) {
        snprintf(
            buffer,
            sizeof(buffer),
            "Power %u %s",
            digiflip_game_power(app->game),
            species ? digiflip_attribute_name(species->attribute) : "");
        canvas_draw_str(canvas, 44, 20, buffer);
        snprintf(
            buffer,
            sizeof(buffer),
            "Wins %u/%u %u%%",
            app->game->total_victories,
            app->game->total_battles,
            digiflip_game_win_percent(app->game));
        canvas_draw_str(canvas, 44, 31, buffer);
        snprintf(
            buffer,
            sizeof(buffer),
            "Round          %u/%u",
            (app->game->battle_round % DIGIFLIP_COLOSSEUM_ROUNDS) + 1U,
            DIGIFLIP_COLOSSEUM_ROUNDS);
        canvas_draw_str(canvas, 44, 42, buffer);
        snprintf(
            buffer,
            sizeof(buffer),
            "Tag round      %u/%u",
            (app->game->tag_round % DIGIFLIP_COLOSSEUM_ROUNDS) + 1U,
            DIGIFLIP_COLOSSEUM_ROUNDS);
        canvas_draw_str(canvas, 44, 53, buffer);
        snprintf(
            buffer,
            sizeof(buffer),
            "Last 15: %u/%u wins",
            digiflip_game_recent_victories(app->game),
            app->game->battle_history_count);
    } else {
        const uint16_t dp = digiflip_game_dp_quarters(app->game);
        const uint16_t dp_max = digiflip_game_dp_max(app->game);
        snprintf(buffer, sizeof(buffer), "DP             %u/%u", dp / 4U, dp_max / 4U);
        canvas_draw_str(canvas, 44, 20, buffer);
        snprintf(buffer, sizeof(buffer), "Injuries       %u", digiflip_game_injuries(app->game));
        canvas_draw_str(canvas, 44, 31, buffer);
        if(species && species->sleep_minutes != DIGIFLIP_NO_SLEEP) {
            snprintf(
                buffer,
                sizeof(buffer),
                "Bedtime        %u:%02u",
                species->sleep_minutes / 60U,
                species->sleep_minutes % 60U);
        } else {
            snprintf(buffer, sizeof(buffer), "Bedtime        --");
        }
        canvas_draw_str(canvas, 44, 42, buffer);
        const char* state = digiflip_game_is_asleep(app->game)  ? "Asleep" :
                            digiflip_game_is_injured(app->game) ? "Injured" :
                            digiflip_game_is_tired(app->game)   ? "Tired" :
                                                                  "Awake";
        snprintf(buffer, sizeof(buffer), "State          %s", state);
        canvas_draw_str(canvas, 44, 53, buffer);
        switch(digiflip_game_call_reason(app->game)) {
        case DigiflipCallHunger:
            snprintf(buffer, sizeof(buffer), "CALL: hungry");
            break;
        case DigiflipCallStrength:
            snprintf(buffer, sizeof(buffer), "CALL: weak");
            break;
        case DigiflipCallTired:
            snprintf(buffer, sizeof(buffer), "CALL: sleepy");
            break;
        default:
            snprintf(buffer, sizeof(buffer), "No call");
            break;
        }
    }
    canvas_draw_line(canvas, 0, 54, 127, 54);
    canvas_draw_str_aligned(canvas, 64, 63, AlignCenter, AlignBottom, buffer);
}

/* Step the card by `delta`, moving into the other pet's cards at either
   end when both are on screen. */
static void digiflip_status_step(DigiflipApp* app, int8_t delta) {
    uint8_t targets[2];
    const bool both = digiflip_care_targets(app, targets) == 2;
    const uint8_t pages = both ? 8U : 4U;
    uint8_t index = (uint8_t)(digiflip_focus_slot(app) * 4U + app->status_page);
    if(!both) index = app->status_page;
    index = (uint8_t)((index + pages + delta) % pages);
    app->status_page = index % 4U;
    if(both) app->game = &app->pets[index / 4U];
}

void digiflip_status_input(DigiflipApp* app, const InputEvent* input) {
    uint8_t targets[2];
    if(input->key == InputKeyBack) {
        digiflip_refocus(app);
        app->screen = app->status_from_slot ? DigiflipScreenSlot : DigiflipScreenHome;
        app->status_from_slot = false;
    } else if(
        (input->key == InputKeyUp || input->key == InputKeyDown) &&
        digiflip_care_targets(app, targets) == 2) {
        app->game = &app->pets[1U - digiflip_focus_slot(app)];
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyLeft) {
        digiflip_status_step(app, -1);
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyRight || input->key == InputKeyOk) {
        digiflip_status_step(app, 1);
        digiflip_play_click(app, input);
    }
}
