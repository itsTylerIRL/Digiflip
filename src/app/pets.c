#include "digiflip_app.h"

bool digiflip_slot_raised(const DigiflipApp* app, uint8_t slot) {
    return slot == 0 || (slot == 1 && app->slot2_kind == DigiflipSlotPet);
}

bool digiflip_slot_filled(const DigiflipApp* app, uint8_t slot) {
    return slot == 0 || (slot == 1 && app->slot2_kind != DigiflipSlotEmpty);
}

bool digiflip_slot_visible(const DigiflipApp* app, uint8_t slot) {
    if(!digiflip_slot_filled(app, slot)) return false;
    switch(app->view) {
    case DigiflipViewOne:
        return slot == 0;
    case DigiflipViewTwo:
        return slot == 1;
    case DigiflipViewBoth:
    default:
        return true;
    }
}

DigiflipSpeciesId digiflip_slot_species(const DigiflipApp* app, uint8_t slot) {
    if(slot == 1 && app->slot2_kind == DigiflipSlotCopymon) return app->copymon;
    if(!digiflip_slot_raised(app, slot)) return DIGIFLIP_SPECIES_NONE;
    return app->pets[slot].species_id;
}

uint8_t digiflip_focus_slot(const DigiflipApp* app) {
    return app->game == &app->pets[1] ? 1U : 0U;
}

uint8_t digiflip_care_targets(const DigiflipApp* app, uint8_t targets[2]) {
    uint8_t count = 0;
    for(uint8_t slot = 0; slot < 2; slot++) {
        if(digiflip_slot_visible(app, slot) && digiflip_slot_raised(app, slot)) {
            targets[count++] = slot;
        }
    }
    return count;
}

DigiflipSpeciesId digiflip_mate_species(const DigiflipApp* app) {
    return digiflip_slot_species(app, (uint8_t)(1U - digiflip_focus_slot(app)));
}

DigiflipGame* digiflip_mate_game(DigiflipApp* app) {
    const uint8_t other = (uint8_t)(1U - digiflip_focus_slot(app));
    return digiflip_slot_raised(app, other) ? &app->pets[other] : NULL;
}

/* Waste piles sit on the far side of their own pet: pet 1's on the left
   when the pair share the screen, otherwise on the right. */
static bool digiflip_poop_on_left(const DigiflipApp* app, uint8_t slot) {
    return app->view == DigiflipViewBoth && slot == 0;
}

int16_t digiflip_poop_x(const DigiflipApp* app, uint8_t slot, uint8_t index) {
    const int16_t offset = (int16_t)(index * DIGIFLIP_POOP_PITCH);
    return digiflip_poop_on_left(app, slot) ? 1 + offset : 128 - 1 - 8 - offset;
}

/* Room a pet's own waste takes from its outer edge (0 when clean). */
static int16_t digiflip_poop_room(const DigiflipApp* app, uint8_t slot) {
    if(!digiflip_slot_raised(app, slot) || app->pets[slot].messes == 0) return 0;
    return (int16_t)(app->pets[slot].messes * DIGIFLIP_POOP_PITCH + DIGIFLIP_POOP_MARGIN);
}

/* Alone, a pet walks the whole field left of its waste. Side by side, each
   pet's waste takes room from its outer edge and the rest splits into two
   halves that never overlap; with no room left, a pet stands against the
   middle over its own piles. */
void digiflip_lane(const DigiflipApp* app, uint8_t slot, int16_t* min_x, int16_t* max_x) {
    if(app->view != DigiflipViewBoth) {
        *min_x = DIGIFLIP_PET_MIN_X;
        *max_x = DIGIFLIP_PET_MAX_X - digiflip_poop_room(app, slot);
        if(*max_x < *min_x) *max_x = *min_x;
        return;
    }
    const int16_t left = 1 + digiflip_poop_room(app, 0);
    const int16_t right = 127 - digiflip_poop_room(app, 1);
    const int16_t middle = (left + right) / 2;
    if(slot == 0) {
        *max_x = middle - 1 - DIGIFLIP_PET_WIDTH;
        *min_x = left < *max_x ? left : *max_x;
    } else {
        *min_x = middle + 1;
        *max_x = right + 1 - DIGIFLIP_PET_WIDTH;
        if(*max_x < *min_x) *max_x = *min_x;
    }
}

void digiflip_refocus(DigiflipApp* app) {
    app->game = app->view == DigiflipViewTwo && app->slot2_kind == DigiflipSlotPet ?
                    &app->pets[1] :
                    &app->pets[0];
}

void digiflip_set_view(DigiflipApp* app, DigiflipView view) {
    if(view != DigiflipViewOne && app->slot2_kind == DigiflipSlotEmpty) view = DigiflipViewOne;
    app->view = view;
    digiflip_refocus(app);
    for(uint8_t slot = 0; slot < 2; slot++) {
        int16_t min_x;
        int16_t max_x;
        digiflip_lane(app, slot, &min_x, &max_x);
        app->views[slot].x = (int16_t)((min_x + max_x) / 2);
    }
}

void digiflip_cycle_view(DigiflipApp* app) {
    if(app->slot2_kind == DigiflipSlotEmpty) {
        digiflip_set_view(app, DigiflipViewOne);
        return;
    }
    digiflip_set_view(app, (DigiflipView)((app->view + 1U) % 3U));
}

const char* digiflip_view_name(DigiflipView view) {
    switch(view) {
    case DigiflipViewBoth:
        return "1 & 2";
    case DigiflipViewTwo:
        return "2";
    default:
        return "1";
    }
}

void digiflip_set_slot2(DigiflipApp* app, DigiflipSlotKind kind, DigiflipSpeciesId copymon) {
    if(kind != DigiflipSlotPet) memset(&app->pets[1], 0, sizeof(app->pets[1]));
    app->slot2_kind = kind;
    app->copymon = kind == DigiflipSlotCopymon ? copymon : DIGIFLIP_SPECIES_NONE;
    app->album_last_species[1] = DIGIFLIP_SPECIES_NONE;
    app->views[1].reaction = DigiflipReactionNone;
    digiflip_slot2_save(kind, app->copymon);
    if(kind == DigiflipSlotPet) digiflip_storage_save_slot(&app->pets[1], 1);
    digiflip_set_view(app, kind == DigiflipSlotEmpty ? DigiflipViewOne : app->view);
}

typedef enum {
    DigiflipSlotActionStats,
    DigiflipSlotActionShowOnly,
    DigiflipSlotActionNewEgg,
    DigiflipSlotActionCopymon,
    DigiflipSlotActionEmpty,
} DigiflipSlotAction;

#define DIGIFLIP_SLOT_ACTIONS_MAX 5U

/* What the slot screen offers: care for a raised pet, then what can replace
   it. Slot 1 always keeps a raised pet, so only slot 2 takes a Copymon or
   can be emptied. */
static uint8_t digiflip_slot_actions(const DigiflipApp* app, DigiflipSlotAction out[]) {
    const uint8_t slot = app->slot_index;
    uint8_t count = 0;
    if(digiflip_slot_raised(app, slot)) out[count++] = DigiflipSlotActionStats;
    if(digiflip_slot_filled(app, slot)) out[count++] = DigiflipSlotActionShowOnly;
    out[count++] = DigiflipSlotActionNewEgg;
    if(slot == 1) {
        out[count++] = DigiflipSlotActionCopymon;
        if(app->slot2_kind != DigiflipSlotEmpty) out[count++] = DigiflipSlotActionEmpty;
    }
    return count;
}

static bool digiflip_slot2_has_living_pet(const DigiflipApp* app) {
    return app->slot2_kind == DigiflipSlotPet && app->pets[1].alive;
}

void digiflip_slot_open(DigiflipApp* app, uint8_t slot) {
    app->slot_index = slot;
    app->settings_index = 0;
    app->slot2_confirm = false;
    app->screen = DigiflipScreenSlot;
}

const char* digiflip_slot_label(const DigiflipApp* app, uint8_t slot) {
    if(digiflip_slot_raised(app, slot)) return digiflip_game_species_name(&app->pets[slot]);
    const DigiflipSpecies* species = digiflip_species_get(digiflip_slot_species(app, slot));
    return species ? species->name : "Empty";
}

void digiflip_draw_slot_menu(Canvas* canvas, const DigiflipApp* app) {
    DigiflipSlotAction actions[DIGIFLIP_SLOT_ACTIONS_MAX];
    const uint8_t count = digiflip_slot_actions(app, actions);
    const uint8_t slot = app->slot_index;
    const char* labels[DIGIFLIP_SLOT_ACTIONS_MAX];
    for(uint8_t i = 0; i < count; i++) {
        switch(actions[i]) {
        case DigiflipSlotActionStats:
            labels[i] = "Stats";
            break;
        case DigiflipSlotActionShowOnly:
            labels[i] = digiflip_slot_raised(app, slot) ? "Show only this pet" :
                                                          "Show only this one";
            break;
        case DigiflipSlotActionNewEgg:
            labels[i] = "Raise a new egg";
            break;
        case DigiflipSlotActionCopymon:
            labels[i] = "Use a Copymon";
            break;
        case DigiflipSlotActionEmpty:
            labels[i] = "Empty the slot";
            break;
        }
    }
    char title[40];
    if(app->slot2_confirm) {
        snprintf(title, sizeof(title), "OK again: empty it?");
    } else if(slot == 1 && app->slot2_kind == DigiflipSlotCopymon) {
        snprintf(title, sizeof(title), "SLOT 2: %.12s copy", digiflip_slot_label(app, slot));
    } else {
        snprintf(
            title, sizeof(title), "SLOT %u: %.16s", slot + 1U, digiflip_slot_label(app, slot));
    }
    digiflip_draw_list(canvas, title, labels, NULL, count, app->settings_index);
}

void digiflip_slot_menu_input(DigiflipApp* app, const InputEvent* input) {
    DigiflipSlotAction actions[DIGIFLIP_SLOT_ACTIONS_MAX];
    const uint8_t count = digiflip_slot_actions(app, actions);
    const uint8_t slot = app->slot_index;
    if(app->settings_index >= count) app->settings_index = 0;
    if(input->key == InputKeyUp || input->key == InputKeyDown) {
        app->settings_index =
            (uint8_t)((app->settings_index + (input->key == InputKeyUp ? count - 1U : 1U)) %
                      count);
        app->slot2_confirm = false;
        digiflip_play_click(app, input);
    } else if(input->key == InputKeyBack) {
        app->slot2_confirm = false;
        app->screen = DigiflipScreenPause;
    } else if(input->key == InputKeyOk) {
        switch(actions[app->settings_index]) {
        case DigiflipSlotActionStats:
            app->game = &app->pets[slot];
            app->status_page = 0;
            app->status_from_slot = true;
            app->screen = DigiflipScreenStatus;
            break;
        case DigiflipSlotActionShowOnly:
            digiflip_set_view(app, slot == 0 ? DigiflipViewOne : DigiflipViewTwo);
            app->screen = DigiflipScreenHome;
            break;
        case DigiflipSlotActionNewEgg:
            /* The egg picker confirms replacing a living pet; Back keeps it. */
            app->egg_slot = slot;
            app->egg_confirm = false;
            app->selected_egg = digiflip_slot_raised(app, slot) ? app->pets[slot].egg_id : 0;
            app->egg_select_from_home = true;
            app->screen = DigiflipScreenEggSelect;
            break;
        case DigiflipSlotActionCopymon:
            digiflip_copymon_open(app);
            break;
        case DigiflipSlotActionEmpty:
            if(digiflip_slot2_has_living_pet(app) && !app->slot2_confirm) {
                app->slot2_confirm = true;
                digiflip_sound_play(app->notification, DigiflipSoundFailure);
                return;
            }
            digiflip_set_slot2(app, DigiflipSlotEmpty, DIGIFLIP_SPECIES_NONE);
            app->slot2_confirm = false;
            app->settings_index = 0;
            break;
        }
        digiflip_play_click(app, input);
    }
}
