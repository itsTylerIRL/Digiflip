#pragma once

#include "digiflip_log.h"
#include "digiflip_roster.h"

#include <stdbool.h>
#include <stdint.h>

#define DIGIFLIP_MAX_HEARTS                 4U
/* One hunger and one strength heart lost per interval. */
#define DIGIFLIP_HEART_DECAY_SECONDS        (30U * 60U) /* at Child; see heart_interval */
/* Waste drop interval. */
#define DIGIFLIP_POOP_INTERVAL_SECONDS      (30U * 60U) /* at Child; see poop_interval */
/* Needs by life stage: babies get hungry and poop the most, slowing through
   adulthood, until old age (6+ days) speeds them up again. */
#define DIGIFLIP_OLD_AGE_DAYS               6U
/* An unanswered call goes out after 10 minutes: one care mistake. */
#define DIGIFLIP_CALL_TIMEOUT_SECONDS       (10U * 60U)
/* A tired Digimon must be put to bed within 30 minutes. */
#define DIGIFLIP_TIRED_CALL_TIMEOUT_SECONDS (30U * 60U)
/* Naps wake automatically after 3 hours; 3h+ of sleep restores DP. */
#define DIGIFLIP_NAP_SECONDS                (3U * 3600U)
#define DIGIFLIP_INJURY_DEATH_SECONDS       (6U * 3600U)
#define DIGIFLIP_STARVE_DEATH_SECONDS       (12U * 3600U)
#define DIGIFLIP_NEGLECT_AWAKE_SECONDS      (48U * 3600U)
/* DP: max 14 at Child, +14 per stage above it, tracked in quarter points. */
#define DIGIFLIP_DP_PER_STAGE               14U
#define DIGIFLIP_DP_QUARTERS_PER_DP         4U
/* One battle costs 1 DP. */
#define DIGIFLIP_BATTLE_DP_QUARTERS         4U
/* Injury chance per battle, win vs loss. */
#define DIGIFLIP_INJURY_CHANCE_WIN_PCT      25U
#define DIGIFLIP_INJURY_CHANCE_LOSS_PCT     50U
/* Power = base + 4 per strength heart (bonus capped at 16); the whole
   strength bonus is lost at 99G. Attribute advantage adds 32. */
#define DIGIFLIP_STRENGTH_POWER_PER_HEART   4U
#define DIGIFLIP_STRENGTH_POWER_MAX         16U
#define DIGIFLIP_MAX_WEIGHT                 99U
#define DIGIFLIP_ATTRIBUTE_BONUS            32U
/* Single training succeeds with 13+ presses in the window and grants 2
   strength hearts; success sheds 1-4G. */
#define DIGIFLIP_TRAIN_PRESSES_NEEDED       13U
#define DIGIFLIP_TRAIN_STRENGTH_GAIN        2U
#define DIGIFLIP_TRAIN_WINDOW_MS            3000U
/* Battle charge: mash for the window to fill the meter; a full meter gives
   the strongest attack (Critical). */
#define DIGIFLIP_BATTLE_CHARGE_MS           3000U
#define DIGIFLIP_BATTLE_CHARGE_PRESSES      16U
/* Exchange model: ranks deal 3-7, plus a bonus at max strength. An evenly
   matched, fully fed Digimon wins ~55% with a Strong and ~88% with a
   Critical. */
#define DIGIFLIP_BATTLE_HP                  24U
#define DIGIFLIP_BATTLE_ENEMY_DAMAGE        5U
#define DIGIFLIP_BATTLE_STRENGTH_DAMAGE     1U
/* Tag battles: each team shares one HP pool, max strength adds +2 damage
   per Digimon, and the aim step moves the attack rank up or down. */
#define DIGIFLIP_TAG_BATTLE_HP              36U
#define DIGIFLIP_TAG_STRENGTH_DAMAGE        2U
#define DIGIFLIP_TAG_TRAIN_ROUNDS           5U
#define DIGIFLIP_TAG_TRAIN_NEEDED           3U
#define DIGIFLIP_BATTLE_MAX_EXCHANGES       40U
#define DIGIFLIP_MAX_INJURIES               20U
/* Stage IV -> V: 12+ of the last 15 battles won is certain. With 6-11 wins
   the route may still be taken, at 10% per win over 5 (traited: +10%). */
#define DIGIFLIP_STAGE_V_SURE_WINS          12U
#define DIGIFLIP_STAGE_V_CHANCE_MIN_WINS    6U
#define DIGIFLIP_STAGE_V_CHANCE_PER_WIN     10U
#define DIGIFLIP_TRAIT_BONUS_PCT            10U
/* Awake this long after its last evolution, a Digimon leaves a traited egg. */
#define DIGIFLIP_TRAIT_AWAKE_SECONDS        (48U * 3600U)
#define DIGIFLIP_MAX_CARE_MISTAKES          20U

typedef enum {
    DigiflipCallNone = 0,
    DigiflipCallHunger = 1,
    DigiflipCallStrength = 2,
    DigiflipCallTired = 4,
} DigiflipCallReason;

typedef enum {
    DigiflipDeathNone = 0,
    DigiflipDeathInjuries, /* 20 injuries in one form */
    DigiflipDeathCareMistakes, /* 20 care mistakes in one form */
    DigiflipDeathNeglect, /* 5 mistakes after 48h awake in this stage */
    DigiflipDeathInjuredTooLong, /* injured for 6 hours */
    DigiflipDeathStarved, /* empty hunger or strength hearts for 12 hours */
} DigiflipDeathCause;

/* Attack rank from the charge meter, weakest to strongest. */
typedef enum {
    DigiflipAttackWeak,
    DigiflipAttackStrong,
    DigiflipAttackDoubleWeak,
    DigiflipAttackDoubleStrong,
    DigiflipAttackCritical,
} DigiflipAttack;

/* Who's attacking in an exchange. Single battles use only the pet and the
   left (only) opponent. */
typedef enum {
    DigiflipFighterPet,
    DigiflipFighterPartner, /* Copymon, in tag battles */
    DigiflipFighterLeft,
    DigiflipFighterRight,
} DigiflipFighter;

typedef struct {
    bool by_player;
    bool hit;
    uint8_t damage;
    uint8_t attacker; /* DigiflipFighter */
} DigiflipBattleExchange;

/* Complete outcome of one battle, resolved up front so the UI can replay it. */
typedef struct {
    uint8_t round; /* Colosseum round fought, 0-based */
    DigiflipSpeciesId player_species; /* form that fought (before any evolution) */
    const DigiflipColosseumOpponent* opponent;
    /* Tag battles: the Copymon partner and the second opponent. */
    bool tag;
    DigiflipSpeciesId partner_species;
    const DigiflipColosseumOpponent* opponent2;
    DigiflipAttack attack;
    uint8_t hit_rate; /* player's per-attack hit chance, percent */
    uint8_t exchange_count;
    DigiflipBattleExchange exchanges[DIGIFLIP_BATTLE_MAX_EXCHANGES];
    bool victory;
    bool injured;
    bool evolved;
    /* Tag battles with a raised teammate (second pet slot). */
    bool mate_injured;
    bool mate_evolved;
} DigiflipBattleResult;

typedef struct {
    uint32_t born_at;
    uint32_t last_update_at;
    /* Continuous clocks: advance only while awake (frozen during sleep). */
    uint32_t stage_elapsed;
    uint32_t awake_seconds;
    /* Absolute event schedule (0 = unscheduled, repaired lazily by tick). */
    uint32_t next_heart_at;
    uint32_t next_poop_at;
    uint32_t rng_state;
    uint16_t trainings;
    uint16_t care_mistakes;
    uint16_t overfeeds;
    uint16_t battles;
    uint16_t battle_history;
    uint16_t tag_battles;
    uint16_t dp_quarters;
    DigiflipSpeciesId species_id;
    DigiflipSpeciesId tag_partner;
    uint32_t injured_at;
    uint32_t hunger_empty_since; /* 0 = meter not empty */
    uint32_t strength_empty_since; /* 0 = meter not empty */
    uint32_t sleep_started_at; /* 0 = awake */
    uint32_t sleep_wake_at;
    uint32_t tired_day; /* day index of last tired trigger, UINT32_MAX = none */
    uint32_t call_deadline; /* absolute */
    uint8_t battle_history_count;
    uint8_t egg_id; /* DigiflipEgg */
    uint8_t stage; /* DigiflipStage */
    uint8_t hunger;
    uint8_t strength;
    uint8_t effort;
    uint8_t weight;
    uint8_t messes;
    uint8_t injuries; /* this form */
    uint8_t call_reason; /* DigiflipCallReason */
    uint8_t call_latch; /* bitmask of DigiflipCallReason: no repeat call */
    uint8_t death_cause; /* DigiflipDeathCause */
    uint16_t evo_from_species; /* species before last evolution; NONE = hatched */
    bool evo_pending; /* UI has not played the evolution animation yet */
    bool lights_on;
    bool injured;
    bool tired;
    bool alive;
    bool overfeed_counted;
    bool evolution_attempted;
    /* Added in save v5; everything from here on is zeroed when migrating. */
    uint16_t total_battles; /* lifetime, all forms */
    uint16_t total_victories;
    uint8_t battle_round; /* next Colosseum round, 0-based */
    /* Added in save v6. */
    uint8_t tag_round; /* next tag Colosseum round, 0-based */
    /* Added in save v7. */
    bool traited; /* hatched from a traited egg */
    bool trait_earned; /* died after 48h awake since its last evolution */
} DigiflipGame;

/* First field added after each save version (see digiflip_storage.c): a v4
   or v5 save is migrated by zeroing everything from its tail onward. */
#define DIGIFLIP_GAME_V4_TAIL total_battles
#define DIGIFLIP_GAME_V5_TAIL tag_round
#define DIGIFLIP_GAME_V6_TAIL traited

void digiflip_game_new(DigiflipGame* game, uint32_t now);
/* A traited egg gives its Digimon a better Stage V chance. */
void digiflip_game_new_with_egg(DigiflipGame* game, uint32_t now, DigiflipEgg egg, bool traited);
/* Advance the simulation to `now` (seconds). Events fire in chronological
   order, including everything that happened while the app was closed. */
void digiflip_game_tick(DigiflipGame* game, uint32_t now);
bool digiflip_game_try_evolve(DigiflipGame* game);
/* Chance in percent that a Stage IV Digimon short of the sure win count still
   evolves, for `victories` of its last 15 battles; 0 when it can't. */
uint8_t digiflip_stage_v_chance(uint8_t victories, bool traited);
bool digiflip_game_feed_meat(DigiflipGame* game);
bool digiflip_game_feed_protein(DigiflipGame* game);
/* Whether this food would add a heart right now (meat: hunger, protein:
   strength). Feeding two pets at once skips the one it wouldn't help. */
bool digiflip_game_food_fills(const DigiflipGame* game, DigiflipFood food);
/* Single training: `presses` is how many times A was hit in the window.
   Every session counts toward effort; returns true on success. */
bool digiflip_game_train(DigiflipGame* game, uint16_t presses);
bool digiflip_game_clean(DigiflipGame* game);
/* Put to sleep (night sleep when tired, else a 3h nap; a nap crossing bedtime
   becomes night sleep). Wake up manually before the automatic wake time. */
bool digiflip_game_sleep(DigiflipGame* game, uint32_t now);
bool digiflip_game_wake(DigiflipGame* game, uint32_t now);
bool digiflip_game_is_asleep(const DigiflipGame* game);
/* Administer medicine: 50% chance per dose to cure an injury. Returns true
   when this dose cured the injury. */
bool digiflip_game_heal(DigiflipGame* game);
bool digiflip_game_is_injured(const DigiflipGame* game);
bool digiflip_game_is_tired(const DigiflipGame* game);
DigiflipCallReason digiflip_game_call_reason(const DigiflipGame* game);
uint16_t digiflip_game_dp_quarters(const DigiflipGame* game);
uint16_t digiflip_game_dp_max(const DigiflipGame* game);
uint8_t digiflip_game_injuries(const DigiflipGame* game);
DigiflipDeathCause digiflip_game_death_cause(const DigiflipGame* game);
const char* digiflip_game_death_cause_name(DigiflipDeathCause cause);
bool digiflip_game_record_battle(DigiflipGame* game, bool victory, DigiflipSpeciesId tag_partner);
/* Why a battle can't start right now (DigiflipBattleOk when it can). */
typedef enum {
    DigiflipBattleOk,
    DigiflipBattleTooYoung,
    DigiflipBattleAsleep,
    DigiflipBattleInjured,
    DigiflipBattleNoDp,
} DigiflipBattleBlock;
DigiflipBattleBlock digiflip_game_battle_block(const DigiflipGame* game);
/* Battle power: base + strength bonus (lost at 99G). */
uint16_t digiflip_game_power(const DigiflipGame* game);
DigiflipAttack digiflip_battle_attack_for_presses(uint16_t presses);
const char* digiflip_battle_attack_name(DigiflipAttack attack);
/* Fight the current Colosseum round. Costs 1 DP; a win advances the round
   (wrapping after the last). Returns false when the battle is blocked.
   `result` (nullable) receives the full exchange log. */
bool digiflip_game_battle(
    DigiflipGame* game,
    uint16_t charge_presses,
    DigiflipBattleResult* result);
const DigiflipColosseumOpponent* digiflip_game_next_opponent(const DigiflipGame* game);
/* Lifetime win percentage, 0 when no battles yet. */
uint8_t digiflip_game_win_percent(const DigiflipGame* game);
uint16_t digiflip_game_age_days(const DigiflipGame* game, uint32_t now);
uint8_t digiflip_game_recent_victories(const DigiflipGame* game);
const char* digiflip_game_stage_name(const DigiflipGame* game);
const char* digiflip_game_species_name(const DigiflipGame* game);
bool digiflip_game_evo_pending(const DigiflipGame* game);
DigiflipSpeciesId digiflip_game_evo_from_species(const DigiflipGame* game);
void digiflip_game_clear_evo_pending(DigiflipGame* game);

/* Receives every event the rules produce (see digiflip_log.h), including
   ones replayed from offline time, stamped with when they happened. */
typedef void (
    *DigiflipEventSink)(void* context, const DigiflipGame* game, const DigiflipEvent* event);
void digiflip_game_set_event_sink(DigiflipEventSink sink, void* context);

/* Cheats. They apply to every game until changed. */
typedef struct {
    bool freeze_hearts; /* hunger and strength hearts never drop */
    bool no_waste; /* no more poop */
} DigiflipRules;
void digiflip_game_set_rules(const DigiflipRules* rules);
void digiflip_game_fill_hearts(DigiflipGame* game);
/* Evolve right now: into the route the current stats qualify for, or the
   species' first route if none do (tag partners waived). Hatches an egg.
   Returns false for a final form. */
bool digiflip_game_force_evolve(DigiflipGame* game);

/* Seconds between lost hearts / poops for this pet right now, by stage and
   age (see DIGIFLIP_OLD_AGE_DAYS). */
uint32_t digiflip_game_heart_interval(const DigiflipGame* game, uint32_t now);
uint32_t digiflip_game_poop_interval(const DigiflipGame* game, uint32_t now);

/* Turn the lights off over a pet that fell asleep with them on. */
bool digiflip_game_lights_off(DigiflipGame* game);

/* How well the tag charge cursor was stopped on its target. */
typedef enum {
    DigiflipSyncMiss, /* -1 attack rank */
    DigiflipSyncNear, /* no change */
    DigiflipSyncPerfect, /* +1 attack rank */
} DigiflipSync;

/* Fight the current tag Colosseum round with `partner` (a Copymon) at the
   pet's side. Same costs and blocks as a single battle; a win advances the
   tag round. Counts toward the partner's Jogress (tag-battle) routes. */
bool digiflip_game_tag_battle(
    DigiflipGame* game,
    DigiflipSpeciesId partner,
    uint16_t charge_presses,
    DigiflipSync sync,
    DigiflipBattleResult* result);
/* The same with a raised teammate (the second pet slot) instead of a
   Copymon: `mate_game` fights with its own hearts, pays DP, can be injured,
   and counts the battle toward its own Jogress routes with the lead as its
   partner. Fails if either can't battle. */
bool digiflip_game_tag_battle_team(
    DigiflipGame* game,
    DigiflipGame* mate_game,
    DigiflipSpeciesId partner,
    uint16_t charge_presses,
    DigiflipSync sync,
    DigiflipBattleResult* result);
const DigiflipTagRound* digiflip_game_next_tag_round(const DigiflipGame* game);

/* Tag training: the guard for the next high/low shot (true = guarding
   high), and the result after DIGIFLIP_TAG_TRAIN_ROUNDS shots. 3+ hits
   succeeds with the same reward as single training. */
bool digiflip_game_tag_guard_high(DigiflipGame* game);
bool digiflip_game_tag_train(DigiflipGame* game, uint8_t hits);
