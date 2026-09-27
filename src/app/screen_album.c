#include "digiflip_app.h"

static int16_t digiflip_draw_tab(Canvas* canvas, int16_t right, const char* label, bool active) {
    const int16_t width = (int16_t)canvas_string_width(canvas, label) + 6;
    const int16_t x = right - width;
    if(active) {
        canvas_draw_rbox(canvas, x, 1, width, 11, 2);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_draw_rframe(canvas, x, 1, width, 11, 2);
    }
    canvas_draw_str(canvas, x + 3, 10, label);
    canvas_set_color(canvas, ColorBlack);
    return x - 2;
}

void digiflip_draw_album(Canvas* canvas, const DigiflipApp* app) {
    char buffer[32];
    const DigiflipSpeciesId id = app->album_index;
    const DigiflipSpecies* species = digiflip_species_get(id);
    const bool seen = digiflip_album_has(&app->album, id);
    const uint16_t owned = digiflip_album_count(&app->album);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "ALBUM");
    canvas_set_font(canvas, FontSecondary);
    snprintf(buffer, sizeof(buffer), "Owned %u", owned);
    const int16_t next = digiflip_draw_tab(canvas, 127, buffer, app->album_owned_only);
    digiflip_draw_tab(canvas, next, "All", !app->album_owned_only);
    canvas_draw_line(canvas, 0, 13, 127, 13);

    if(app->album_owned_only && owned == 0) {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "No Digimon raised yet");
        canvas_draw_str_aligned(canvas, 64, 46, AlignCenter, AlignCenter, "OK: show all");
        return;
    }

    if(seen) {
        digiflip_draw_character(canvas, 4, 15, id, DigiflipPoseIdle, false);
    } else {
        for(int16_t i = 8; i < 36; i += 3) {
            canvas_draw_dot(canvas, 4 + i, 19);
            canvas_draw_dot(canvas, 4 + i, 47);
            canvas_draw_dot(canvas, 8, 15 + i);
            canvas_draw_dot(canvas, 36, 15 + i);
        }
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 22, 33, AlignCenter, AlignCenter, "?");
        canvas_set_font(canvas, FontSecondary);
    }

    snprintf(buffer, sizeof(buffer), "No. %03u/%u", id + 1U, (unsigned)digiflip_species_count);
    canvas_draw_str(canvas, 46, 24, buffer);
    if(species && seen) {
        canvas_draw_str(canvas, 46, 35, digiflip_stage_name(species->stage));
        canvas_draw_str(canvas, 46, 46, digiflip_attribute_name(species->attribute));
    } else {
        canvas_draw_str(canvas, 46, 35, "Not raised yet");
    }
    canvas_draw_line(canvas, 0, 52, 127, 52);
    canvas_draw_str_aligned(
        canvas, 64, 63, AlignCenter, AlignBottom, species && seen ? species->name : "???");
}

/* On the Owned tab, land on an owned entry (the nearest one ahead). */
static void digiflip_album_snap(DigiflipApp* app) {
    if(!app->album_owned_only || digiflip_album_has(&app->album, app->album_index)) return;
    const DigiflipSpeciesId next = digiflip_album_step(&app->album, app->album_index, 1);
    if(next != DIGIFLIP_SPECIES_NONE) app->album_index = next;
}

void digiflip_album_open(DigiflipApp* app) {
    if(app->game->species_id != DIGIFLIP_SPECIES_NONE) app->album_index = app->game->species_id;
    digiflip_album_snap(app);
    app->screen = DigiflipScreenAlbum;
}

/* Move `steps` entries (negative = backwards), counting only owned entries
   on the Owned tab. */
static void digiflip_album_move(DigiflipApp* app, int steps) {
    const int count = (int)digiflip_species_count;
    if(app->album_owned_only) {
        for(int i = 0; i < (steps < 0 ? -steps : steps); i++) {
            const DigiflipSpeciesId next =
                digiflip_album_step(&app->album, app->album_index, steps);
            if(next == DIGIFLIP_SPECIES_NONE) return;
            app->album_index = next;
        }
    } else {
        app->album_index = (uint16_t)((((int)app->album_index + steps) % count + count) % count);
    }
}

void digiflip_album_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyLeft || input->key == InputKeyRight) {
        digiflip_album_move(app, input->key == InputKeyLeft ? -1 : 1);
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyUp || input->key == InputKeyDown) {
        const int jump = app->album_owned_only ? 5 : 10;
        digiflip_album_move(app, input->key == InputKeyUp ? -jump : jump);
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyOk) {
        app->album_owned_only = !app->album_owned_only;
        digiflip_album_snap(app);
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenPause;
    }
}

/* Record a raised pet's current form; returns true when it's a first. */
bool digiflip_album_note(DigiflipApp* app, uint8_t slot) {
    if(!digiflip_slot_raised(app, slot)) return false;
    const DigiflipSpeciesId species = app->pets[slot].species_id;
    if(species == DIGIFLIP_SPECIES_NONE || species == app->album_last_species[slot]) return false;
    app->album_last_species[slot] = species;
    if(!digiflip_album_mark(&app->album, species)) return false;
    digiflip_album_save(&app->album);
    return true;
}
