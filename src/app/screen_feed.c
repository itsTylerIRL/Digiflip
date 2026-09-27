#include "digiflip_app.h"

void digiflip_draw_feed(Canvas* canvas, const DigiflipApp* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 35, 13, "FEED");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_icon(canvas, 5, 15, digiflip_effect_sprite(DigiflipEffectMeat));
    canvas_draw_icon(canvas, 5, 34, digiflip_effect_sprite(DigiflipEffectProtein));
    canvas_draw_str(canvas, 31, 30, "Meat");
    canvas_draw_str(canvas, 31, 48, "Protein");
    uint8_t targets[2];
    if(digiflip_care_targets(app, targets) == 2) {
        for(uint8_t slot = 0; slot < 2; slot++) {
            const int16_t dy = slot * 8;
            const char* number = slot ? "2" : "1";
            canvas_draw_str(canvas, 82, 25 + dy, number);
            canvas_draw_str(canvas, 82, 43 + dy, number);
            digiflip_draw_hearts(canvas, 90, 19 + dy, app->pets[slot].hunger);
            digiflip_draw_hearts(canvas, 90, 37 + dy, app->pets[slot].strength);
        }
    } else {
        digiflip_draw_hearts(canvas, 90, 25, app->game->hunger);
        digiflip_draw_hearts(canvas, 90, 43, app->game->strength);
    }
    canvas_draw_str(canvas, 25, app->feed_choice == 0 ? 30 : 48, ">");
    canvas_draw_str(canvas, 6, 62, "OK feed   BACK done");
}

void digiflip_feed_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyUp || input->key == InputKeyDown) {
        app->feed_choice ^= 1U;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk) {
        /* Every awake pet on screen gets a serving, unless the food would
           only fill one of them: then the full one sits it out rather than
           being overfed. */
        const DigiflipFood food = app->feed_choice == 0 ? DigiflipFoodMeat : DigiflipFoodProtein;
        uint8_t targets[2];
        uint8_t eaters[2];
        uint8_t count = 0;
        uint8_t needy = 0;
        const uint8_t shown = digiflip_care_targets(app, targets);
        for(uint8_t i = 0; i < shown; i++) {
            const DigiflipGame* game = &app->pets[targets[i]];
            if(game->stage == DigiflipStageEgg || digiflip_game_is_asleep(game)) continue;
            eaters[count++] = targets[i];
            if(digiflip_game_food_fills(game, food)) needy++;
        }
        bool any_fed = false;
        app->screen = DigiflipScreenHome;
        for(uint8_t i = 0; i < count; i++) {
            const uint8_t slot = eaters[i];
            DigiflipGame* game = &app->pets[slot];
            if(needy > 0 && needy < count && !digiflip_game_food_fills(game, food)) continue;
            const bool fed = app->feed_choice == 0 ? digiflip_game_feed_meat(game) :
                                                     digiflip_game_feed_protein(game);
            int16_t min_x;
            int16_t max_x;
            digiflip_lane(app, slot, &min_x, &max_x);
            app->views[slot].x = app->view != DigiflipViewBoth ? 34 : min_x;
            app->views[slot].direction = 1;
            digiflip_start_reaction(
                app,
                slot,
                !fed                  ? DigiflipReactionRefuse :
                app->feed_choice == 0 ? DigiflipReactionMeat :
                                        DigiflipReactionProtein);
            any_fed |= fed;
        }
        digiflip_sound_play(app->notification, any_fed ? DigiflipSoundFeed : DigiflipSoundFailure);
    }
}
