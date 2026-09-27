#include "digiflip_app.h"

/* 7x6 hearts, one row per byte (bit 6 = leftmost pixel). */
static const uint8_t digiflip_heart_full[6] = {0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08};
static const uint8_t digiflip_heart_empty[6] = {0x36, 0x49, 0x41, 0x22, 0x14, 0x08};

static void digiflip_draw_heart(Canvas* canvas, int16_t x, int16_t y, bool full) {
    const uint8_t* rows = full ? digiflip_heart_full : digiflip_heart_empty;
    for(int16_t row = 0; row < 6; row++) {
        for(int16_t col = 0; col < 7; col++) {
            if(rows[row] & (0x40U >> col)) canvas_draw_dot(canvas, x + col, y + row);
        }
    }
}

void digiflip_draw_hearts(Canvas* canvas, uint8_t x, uint8_t y, uint8_t value) {
    for(uint8_t i = 0; i < DIGIFLIP_MAX_HEARTS; i++) {
        digiflip_draw_heart(canvas, x + i * 9, y, i < value);
    }
}

void digiflip_draw_meter(Canvas* canvas, uint8_t y, uint16_t value, uint16_t full, uint16_t goal) {
    const uint8_t x = 10;
    const uint8_t width = 108;
    canvas_draw_frame(canvas, x, y, width, 8);
    const uint16_t clamped = value > full ? full : value;
    const uint8_t fill = (uint8_t)((uint32_t)clamped * (width - 2U) / full);
    if(fill) canvas_draw_box(canvas, x + 1, y + 1, fill, 6);
    if(goal) {
        const uint8_t gx = (uint8_t)(x + 1U + (uint32_t)goal * (width - 2U) / full);
        canvas_draw_line(canvas, gx, y - 2, gx, y + 9);
    }
}

void digiflip_draw_list(
    Canvas* canvas,
    const char* title,
    const char* const* labels,
    const char* const* values,
    uint8_t count,
    uint8_t selected) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, title);
    canvas_draw_line(canvas, 0, 12, 127, 12);
    canvas_set_font(canvas, FontSecondary);
    const uint8_t visible = 5;
    uint8_t first = 0;
    if(count > visible && selected > 2) {
        first = selected - 2U;
        if(first > count - visible) first = count - visible;
    }
    for(uint8_t row = 0; row < visible && first + row < count; row++) {
        const uint8_t i = first + row;
        const int16_t y = 14 + row * 10;
        if(i == selected) {
            canvas_draw_rbox(canvas, 2, y, 121, 10, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_draw_str(canvas, 8, y + 8, labels[i]);
        if(values && values[i]) {
            canvas_draw_str_aligned(canvas, 118, y + 8, AlignRight, AlignBottom, values[i]);
        }
        canvas_set_color(canvas, ColorBlack);
    }
    if(count > visible) {
        const int16_t track = 64 - 14;
        const int16_t thumb = track * visible / count;
        const int16_t top = 14 + (track - thumb) * first / (count - visible);
        canvas_draw_line(canvas, 126, 14, 126, 63);
        canvas_draw_box(canvas, 125, top, 3, thumb);
    }
}

void digiflip_draw_pet(
    Canvas* canvas,
    const DigiflipApp* app,
    int32_t x,
    int32_t y,
    DigiflipPose pose,
    bool flipped) {
    if(app->game->stage == DigiflipStageEgg) {
        const Icon* egg = digiflip_egg_sprite((DigiflipEgg)app->game->egg_id);
        if(egg) canvas_draw_icon(canvas, x, y, egg);
    } else {
        digiflip_draw_character(canvas, x, y, app->game->species_id, pose, flipped);
    }
}

uint32_t digiflip_mash_remaining(const DigiflipApp* app, uint32_t window) {
    if(app->mash_start_tick == 0) return window;
    const uint32_t elapsed = furi_get_tick() - app->mash_start_tick;
    return elapsed >= window ? 0 : window - elapsed;
}

void digiflip_draw_mash_timer(Canvas* canvas, const DigiflipApp* app, uint32_t window) {
    const uint32_t remaining = digiflip_mash_remaining(app, window);
    canvas_draw_box(canvas, 0, 62, (uint8_t)(remaining * 128U / window), 2);
}
