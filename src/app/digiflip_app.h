#pragma once

#include "../core/digiflip_game.h"
#include "../core/digiflip_unlock.h"
#include "../services/digiflip_sound.h"
#include "../services/digiflip_sprites.h"
#include "../services/digiflip_storage.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    DigiflipScreenEggSelect,
    DigiflipScreenHome,
    DigiflipScreenStatus,
    DigiflipScreenFeed,
    DigiflipScreenTrain,
    DigiflipScreenBattleIntro,
    DigiflipScreenBattleCharge,
    DigiflipScreenBattle,
    DigiflipScreenNotice,
    DigiflipScreenDeath,
    DigiflipScreenPause,
    DigiflipScreenSettings,
    DigiflipScreenAlbum,
    DigiflipScreenLogs,
    DigiflipScreenCheats,
    DigiflipScreenModePick,
    DigiflipScreenTagTrain,
    DigiflipScreenTagSync,
    DigiflipScreenCopymon,
    DigiflipScreenSlot,
} DigiflipScreen;

/* Which pets the home screen shows: slot 1, both, or slot 2. */
typedef enum {
    DigiflipViewOne,
    DigiflipViewBoth,
    DigiflipViewTwo,
} DigiflipView;

typedef enum {
    DigiflipPauseResume,
    DigiflipPauseView,
    DigiflipPauseSlot1,
    DigiflipPauseSlot2,
    DigiflipPauseSettings,
    DigiflipPauseAlbum,
    DigiflipPauseExit,
    DigiflipPauseCount,
} DigiflipPauseItem;

typedef enum {
    DigiflipCheatItemFreezeHearts,
    DigiflipCheatItemNoWaste,
    DigiflipCheatItemFillHearts,
    DigiflipCheatItemForceEvolution,
    DigiflipCheatItemAllEggs,
    DigiflipCheatItemDone,
    DigiflipCheatItemCount,
} DigiflipCheatItem;

typedef enum {
    DigiflipSettingSound,
    DigiflipSettingCallLight,
    DigiflipSettingLogs,
    DigiflipSettingCheats,
    DigiflipSettingDone,
    DigiflipSettingCount,
} DigiflipSettingItem;

typedef enum {
    DigiflipReactionNone,
    DigiflipReactionMeat,
    DigiflipReactionProtein,
    DigiflipReactionRefuse,
    DigiflipReactionCheer, /* waste flushed away */
    DigiflipReactionHealed, /* medicine worked */
    DigiflipReactionNotHealed, /* medicine didn't take this time */
} DigiflipReaction;

typedef enum {
    DigiflipActionStatus,
    DigiflipActionFeed,
    DigiflipActionTrain,
    DigiflipActionBattle,
    DigiflipActionClean,
    DigiflipActionLight,
    DigiflipActionMedicine,
    DigiflipActionCount,
} DigiflipAction;

typedef struct {
    int16_t x;
    int8_t direction;
    DigiflipReaction reaction;
    bool reaction_sound_played;
    uint8_t clean_messes; /* waste still drawn until the flush passes */
} DigiflipPetView;

typedef struct {
    /* Pet slots: slot 0 is always a raised pet; slot 1 holds a second pet, a
       Copymon, or nothing (slot2_kind). `game` is the focused pet. */
    DigiflipGame pets[2];
    DigiflipGame* game;
    DigiflipSlotKind slot2_kind;
    DigiflipView view;
    DigiflipPetView views[2];
    uint8_t egg_slot; /* which slot the egg picker fills */
    uint8_t death_slot; /* whose gravestone is showing */
    uint8_t evo_slot; /* whose evolution is animating */
    uint8_t slot_index; /* which slot the slot screen shows */
    bool status_from_slot; /* Stats opened from the slot screen returns there */
    bool slot2_confirm; /* second OK needed to empty a slot with a living pet */
    bool egg_confirm; /* second OK needed before an egg replaces a living pet */
    DigiflipSettings settings;
    /* Guards everything below against the GUI thread's draw callback. */
    FuriMutex* mutex;
    FuriMessageQueue* queue;
    NotificationApp* notification;
    DigiflipScreen screen;
    uint8_t selected_action;
    uint8_t feed_choice;
    uint8_t selected_egg;
    uint8_t status_page;
    uint8_t pause_index;
    uint8_t settings_index;
    uint8_t pet_anim_phase;
    uint32_t last_frame_tick;
    uint32_t last_pet_step_tick;
    uint32_t reaction_started_tick;
    const char* notice;
    const char* notice_detail;
    char notice_buffer[32];
    char notice_detail_buffer[32];
    uint32_t last_save_tick;
    uint8_t last_call_reason;
    bool call_led_on;
    bool egg_select_from_home;
    /* Mash minigames (training and battle charge): 0 = waiting for first press. */
    uint32_t mash_start_tick;
    uint16_t mash_presses;
    /* Battle replay: the outcome is resolved first, then animated. */
    DigiflipBattleResult battle;
    uint32_t battle_start_tick;
    uint8_t battle_sound_step;
    uint32_t clean_start_tick;
    /* Evolution animation state (0 = not playing). */
    uint32_t evo_anim_start_tick;
    bool evo_album_new; /* the evolving form is a first for the album */
    DigiflipAlbum album;
    DigiflipRecords records;
    uint8_t unlock_notice; /* egg + 1 to announce once home is idle, 0 = none */
    DigiflipSpeciesId album_last_species[2];
    uint16_t album_index;
    bool album_owned_only; /* Owned tab (true) or All tab */
    DigiflipLog log;
    bool log_dirty;
    uint16_t log_index;
    uint8_t cheats_index;
    DigiflipScreen notice_return; /* where OK/Back on a notice goes */
    DigiflipSpeciesId copymon;
    uint16_t copymon_index;
    uint8_t mode_action; /* DigiflipActionTrain or DigiflipActionBattle */
    uint8_t mode_index; /* 0 = single, 1 = tag */
    bool battle_tag;
    DigiflipSync battle_sync;
    int16_t sync_cursor;
    int8_t sync_direction;
    uint8_t tag_shot;
    uint8_t tag_hits;
    bool tag_guard_high;
    bool tag_aim_high;
    uint32_t tag_shot_tick; /* 0 = waiting for a choice */
    bool running;
} DigiflipApp;

/* Home layout: four 12px icons along the top and bottom edges, the play
   field between them. */
#define DIGIFLIP_ICON_SIZE               12
#define DIGIFLIP_ICON_SLOT               32
#define DIGIFLIP_FIELD_TOP               12
#define DIGIFLIP_FIELD_BOTTOM            52
#define DIGIFLIP_CALL_SLOT               DigiflipActionCount
#define DIGIFLIP_PET_Y                   14
#define DIGIFLIP_PET_WIDTH               36 /* 18 dots, doubled */
#define DIGIFLIP_PET_MIN_X               4
#define DIGIFLIP_PET_MAX_X               88
/* Waste piles are 8px wide on a 9px pitch, lined up from the far edge on
   the pet's own side; the pet keeps a 3px margin from them. */
#define DIGIFLIP_POOP_PITCH              9
#define DIGIFLIP_POOP_MARGIN             3
/* Evolution animation: blink old form, flash between old/new, then reveal. */
#define DIGIFLIP_EVO_ANIM_MS             2700U
#define DIGIFLIP_WALK_STEP_TICKS         140U
#define DIGIFLIP_REACTION_APPROACH_TICKS 300U
#define DIGIFLIP_BITE_TICKS              160U
#define DIGIFLIP_REACTION_EAT_TICKS      (DIGIFLIP_REACTION_APPROACH_TICKS + 4U * DIGIFLIP_BITE_TICKS)
#define DIGIFLIP_REACTION_TOTAL_TICKS    (DIGIFLIP_REACTION_EAT_TICKS + 1200U)
#define DIGIFLIP_REFUSE_TICKS            900U
#define DIGIFLIP_CLEAN_ANIM_MS           900U
#define DIGIFLIP_CHEER_TICKS             1300U
#define DIGIFLIP_MEDICINE_APPROACH_TICKS 600U
#define DIGIFLIP_MEDICINE_TOTAL_TICKS    (DIGIFLIP_MEDICINE_APPROACH_TICKS + 1100U)
#define DIGIFLIP_BATTLE_INTRO_MS         500U
#define DIGIFLIP_BATTLE_EXCHANGE_MS      450U
#define DIGIFLIP_BATTLE_IMPACT_MS        300U
#define DIGIFLIP_BATTLE_KO_MS            1400U
#define DIGIFLIP_BATTLE_PLAYER_X         4
#define DIGIFLIP_BATTLE_ENEMY_X          88
#define DIGIFLIP_BATTLE_Y                16

void digiflip_draw_hearts(Canvas* canvas, uint8_t x, uint8_t y, uint8_t value);
void digiflip_draw_meter(Canvas* canvas, uint8_t y, uint16_t value, uint16_t full, uint16_t goal);
void digiflip_draw_list(
    Canvas* canvas,
    const char* title,
    const char* const* labels,
    const char* const* values,
    uint8_t count,
    uint8_t selected);
void digiflip_draw_pet(
    Canvas* canvas,
    const DigiflipApp* app,
    int32_t x,
    int32_t y,
    DigiflipPose pose,
    bool flipped);
uint32_t digiflip_mash_remaining(const DigiflipApp* app, uint32_t window);
void digiflip_draw_mash_timer(Canvas* canvas, const DigiflipApp* app, uint32_t window);

void digiflip_play_click(DigiflipApp* app, const InputEvent* input);
void digiflip_show_notice(DigiflipApp* app, const char* notice);
void digiflip_refuse(DigiflipApp* app, const char* notice);
void digiflip_draw_notice(Canvas* canvas, const DigiflipApp* app);
void digiflip_show_death(DigiflipApp* app, uint8_t slot);
void digiflip_draw_death(Canvas* canvas, const DigiflipApp* app);
void digiflip_notice_input(DigiflipApp* app, const InputEvent* input);
void digiflip_death_input(DigiflipApp* app, const InputEvent* input);

void digiflip_draw_home(Canvas* canvas, const DigiflipApp* app);
bool digiflip_home_busy(const DigiflipApp* app);
void digiflip_handle_home(DigiflipApp* app, const InputEvent* input);
void digiflip_update_pet_walk(DigiflipApp* app, uint32_t tick);
void digiflip_start_reaction(DigiflipApp* app, uint8_t slot, DigiflipReaction reaction);

void digiflip_draw_status(Canvas* canvas, const DigiflipApp* app);
void digiflip_status_input(DigiflipApp* app, const InputEvent* input);

void digiflip_draw_feed(Canvas* canvas, const DigiflipApp* app);
void digiflip_feed_input(DigiflipApp* app, const InputEvent* input);

void digiflip_start_mash(DigiflipApp* app, DigiflipScreen screen);
void digiflip_handle_mash(DigiflipApp* app, const InputEvent* input);
void digiflip_draw_train(Canvas* canvas, const DigiflipApp* app);
void digiflip_finish_training(DigiflipApp* app);

void digiflip_draw_battle_intro(Canvas* canvas, const DigiflipApp* app);
void digiflip_draw_battle_charge(Canvas* canvas, const DigiflipApp* app);
void digiflip_draw_battle(Canvas* canvas, const DigiflipApp* app);
void digiflip_start_battle(DigiflipApp* app);
void digiflip_finish_battle(DigiflipApp* app);
void digiflip_update_battle(DigiflipApp* app, uint32_t tick);
void digiflip_battle_intro_input(DigiflipApp* app, const InputEvent* input);
void digiflip_battle_input(DigiflipApp* app, const InputEvent* input);

void digiflip_draw_pause(Canvas* canvas, const DigiflipApp* app);
void digiflip_draw_settings(Canvas* canvas, const DigiflipApp* app);
void digiflip_handle_pause(DigiflipApp* app, const InputEvent* input);
void digiflip_apply_settings(DigiflipApp* app);
void digiflip_handle_settings(DigiflipApp* app, const InputEvent* input);

void digiflip_draw_egg_select(Canvas* canvas, const DigiflipApp* app);
void digiflip_egg_select_input(DigiflipApp* app, const InputEvent* input);
/* Bit n set when egg n can be picked (every egg under the All eggs cheat). */
uint16_t digiflip_eggs_open(const DigiflipApp* app);
/* Log and queue an announcement for eggs unlocked since last checked. */
void digiflip_check_unlocks(DigiflipApp* app);
void digiflip_record_win(DigiflipApp* app);

void digiflip_draw_album(Canvas* canvas, const DigiflipApp* app);
void digiflip_album_input(DigiflipApp* app, const InputEvent* input);
bool digiflip_album_note(DigiflipApp* app, uint8_t slot);
void digiflip_album_open(DigiflipApp* app);

void digiflip_draw_logs(Canvas* canvas, const DigiflipApp* app);
void digiflip_logs_open(DigiflipApp* app);
void digiflip_logs_input(DigiflipApp* app, const InputEvent* input);
void digiflip_log_event(void* context, const DigiflipGame* game, const DigiflipEvent* event);
void digiflip_log_app_event(DigiflipApp* app, DigiflipEventType type, uint8_t a, uint16_t b);

void digiflip_apply_cheats(const DigiflipApp* app);
void digiflip_draw_cheats(Canvas* canvas, const DigiflipApp* app);
void digiflip_cheats_open(DigiflipApp* app);
void digiflip_cheats_input(DigiflipApp* app, const InputEvent* input);

/* Pick who trains or battles: solo with either pet, or tag led by either. */
void digiflip_mode_open(DigiflipApp* app, DigiflipAction action);
void digiflip_draw_mode_pick(Canvas* canvas, const DigiflipApp* app);
void digiflip_mode_input(DigiflipApp* app, const InputEvent* input);
void digiflip_draw_tag_train(Canvas* canvas, const DigiflipApp* app);
void digiflip_tag_train_input(DigiflipApp* app, const InputEvent* input);
void digiflip_update_tag_train(DigiflipApp* app, uint32_t tick);
void digiflip_draw_tag_sync(Canvas* canvas, const DigiflipApp* app);
void digiflip_tag_sync_input(DigiflipApp* app, const InputEvent* input);
void digiflip_update_tag_sync(DigiflipApp* app);
void digiflip_draw_copymon(Canvas* canvas, const DigiflipApp* app);
void digiflip_copymon_open(DigiflipApp* app);
void digiflip_copymon_input(DigiflipApp* app, const InputEvent* input);

bool digiflip_slot_raised(const DigiflipApp* app, uint8_t slot);
bool digiflip_slot_filled(const DigiflipApp* app, uint8_t slot);
bool digiflip_slot_visible(const DigiflipApp* app, uint8_t slot);
DigiflipSpeciesId digiflip_slot_species(const DigiflipApp* app, uint8_t slot);
uint8_t digiflip_focus_slot(const DigiflipApp* app);
/* Where a pet may stand (its left edge) in the current view. */
void digiflip_lane(const DigiflipApp* app, uint8_t slot, int16_t* min_x, int16_t* max_x);
/* Left edge of waste pile `index` for `slot`: from the far left for pet 1
   side by side, otherwise from the far right. */
int16_t digiflip_poop_x(const DigiflipApp* app, uint8_t slot, uint8_t index);
/* Raised pets the current view's care actions apply to; returns the count. */
uint8_t digiflip_care_targets(const DigiflipApp* app, uint8_t targets[2]);
/* The tag teammate: the other slot's Digimon, and its game if it's raised. */
DigiflipSpeciesId digiflip_mate_species(const DigiflipApp* app);
DigiflipGame* digiflip_mate_game(DigiflipApp* app);
void digiflip_refocus(DigiflipApp* app);
void digiflip_set_view(DigiflipApp* app, DigiflipView view);
void digiflip_cycle_view(DigiflipApp* app);
const char* digiflip_view_name(DigiflipView view);
/* Change what's in slot 2 (saved); leaving the Pet kind forgets pet 2. */
void digiflip_set_slot2(DigiflipApp* app, DigiflipSlotKind kind, DigiflipSpeciesId copymon);
/* The slot screen: stats, view and replacement controls for one slot. */
void digiflip_slot_open(DigiflipApp* app, uint8_t slot);
void digiflip_draw_slot_menu(Canvas* canvas, const DigiflipApp* app);
void digiflip_slot_menu_input(DigiflipApp* app, const InputEvent* input);
/* The slot's Digimon name, or "Empty". */
const char* digiflip_slot_label(const DigiflipApp* app, uint8_t slot);
