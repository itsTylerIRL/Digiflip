#include "digiflip_game.h"

#include <limits.h>
#include <string.h>

/* Event sink (the app's log) and cheat rules. Both are process-wide rather
   than part of DigiflipGame, so the save format doesn't carry them. */
static DigiflipEventSink digiflip_event_sink;
static void* digiflip_event_context;
static DigiflipRules digiflip_rules;
/* While replaying offline time, events are stamped with the moment they
   happened rather than the moment the app caught up. */
static bool digiflip_replaying;
static uint32_t digiflip_replay_now;

void digiflip_game_set_event_sink(DigiflipEventSink sink, void* context) {
    digiflip_event_sink = sink;
    digiflip_event_context = context;
}

void digiflip_game_set_rules(const DigiflipRules* rules) {
    digiflip_rules = *rules;
}

static void digiflip_emit(
    const DigiflipGame* game,
    DigiflipEventType type,
    uint8_t a,
    uint16_t b,
    uint16_t c) {
    if(!digiflip_event_sink) return;
    const DigiflipEvent event = {
        .time = digiflip_replaying ? digiflip_replay_now : game->last_update_at,
        .type = (uint8_t)type,
        .a = a,
        .b = b,
        .c = c,
    };
    digiflip_event_sink(digiflip_event_context, game, &event);
}

static uint32_t digiflip_rng_next(DigiflipGame* game) {
    uint32_t value = game->rng_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    game->rng_state = value ? value : 0xD161F11FU;
    return game->rng_state;
}

static uint32_t digiflip_sat_add32(uint32_t a, uint32_t b) {
    const uint32_t sum = a + b;
    return sum < a ? UINT32_MAX : sum;
}

static bool digiflip_is_asleep(const DigiflipGame* game) {
    return game->sleep_started_at != 0;
}

uint16_t digiflip_game_dp_max(const DigiflipGame* game) {
    if(game->stage < DigiflipStageChild) return 0;
    const uint16_t stages_above_child = (uint16_t)((uint16_t)game->stage - DigiflipStageChild);
    return (uint16_t)((DIGIFLIP_DP_PER_STAGE + DIGIFLIP_DP_PER_STAGE * stages_above_child) *
                      DIGIFLIP_DP_QUARTERS_PER_DP);
}

static void digiflip_game_kill(DigiflipGame* game, DigiflipDeathCause cause) {
    game->alive = false;
    game->trait_earned = game->stage != DigiflipStageEgg &&
                         game->stage_elapsed >= DIGIFLIP_TRAIT_AWAKE_SECONDS;
    game->death_cause = (uint8_t)cause;
    game->call_reason = DigiflipCallNone;
    digiflip_emit(game, DigiflipEventDied, (uint8_t)cause, 0, 0);
}

static void digiflip_game_check_mistake_deaths(DigiflipGame* game) {
    if(game->care_mistakes >= DIGIFLIP_MAX_CARE_MISTAKES) {
        digiflip_game_kill(game, DigiflipDeathCareMistakes);
    } else if(game->care_mistakes >= 5 && game->awake_seconds >= DIGIFLIP_NEGLECT_AWAKE_SECONDS) {
        digiflip_game_kill(game, DigiflipDeathNeglect);
    }
}

static void digiflip_game_injure(DigiflipGame* game, uint32_t now, DigiflipInjuryCause cause) {
    if(game->injuries < UINT8_MAX) game->injuries++;
    game->injured = true;
    game->injured_at = now;
    digiflip_emit(game, DigiflipEventInjured, (uint8_t)cause, game->injuries, 0);
    if(game->injuries >= DIGIFLIP_MAX_INJURIES) digiflip_game_kill(game, DigiflipDeathInjuries);
}

static void digiflip_game_reset_stage_counters(DigiflipGame* game) {
    game->trainings = 0;
    game->care_mistakes = 0;
    game->overfeeds = 0;
    game->battles = 0;
    game->battle_history = 0;
    game->battle_history_count = 0;
    game->tag_battles = 0;
    game->tag_partner = DIGIFLIP_SPECIES_NONE;
    game->effort = 0;
    game->overfeed_counted = false;
    game->evolution_attempted = false;
    game->injuries = 0;
    game->injured = false;
    game->injured_at = 0;
    game->tired = false;
    game->tired_day = UINT32_MAX;
    game->call_reason = DigiflipCallNone;
    game->call_latch = 0;
    game->call_deadline = 0;
    game->awake_seconds = 0;
    game->sleep_started_at = 0;
    game->sleep_wake_at = 0;
    game->lights_on = true;
    game->dp_quarters = digiflip_game_dp_max(game);
    /* Clocks reschedule lazily on the next tick. hunger_empty_since is left
       alone: a meter that was empty before evolution is still empty after. */
    game->next_heart_at = 0;
    game->next_poop_at = 0;
}

static void digiflip_game_set_species(
    DigiflipGame* game,
    DigiflipSpeciesId species_id,
    uint32_t overflow_seconds) {
    const DigiflipSpecies* species = digiflip_species_get(species_id);
    if(!species) return;
    game->species_id = species_id;
    game->stage = (uint8_t)species->stage;
    game->stage_elapsed = overflow_seconds;
    digiflip_game_reset_stage_counters(game);
}

void digiflip_game_new_with_egg(DigiflipGame* game, uint32_t now, DigiflipEgg egg, bool traited) {
    memset(game, 0, sizeof(*game));
    game->born_at = now;
    game->last_update_at = now;
    game->rng_state = now ^ 0xD161F11FU;
    game->species_id = DIGIFLIP_SPECIES_NONE;
    game->tag_partner = DIGIFLIP_SPECIES_NONE;
    game->evo_from_species = DIGIFLIP_SPECIES_NONE;
    game->evo_pending = false;
    game->tired_day = UINT32_MAX;
    game->egg_id = (uint8_t)(egg < DigiflipEggCount ? egg : DigiflipEggVersion1);
    game->stage = (uint8_t)DigiflipStageEgg;
    game->weight = 5;
    game->lights_on = true;
    game->alive = true;
    game->traited = traited;
    digiflip_emit(game, DigiflipEventNewEgg, game->egg_id, traited ? 1U : 0U, 0);
}

void digiflip_game_new(DigiflipGame* game, uint32_t now) {
    digiflip_game_new_with_egg(game, now, DigiflipEggVersion1, false);
}

uint8_t digiflip_game_recent_victories(const DigiflipGame* game) {
    uint16_t history = game->battle_history;
    uint8_t victories = 0;
    for(uint8_t i = 0; i < game->battle_history_count; i++) {
        victories += (uint8_t)(history & 1U);
        history >>= 1;
    }
    return victories;
}

static DigiflipEvolutionContext digiflip_game_evolution_context(const DigiflipGame* game) {
    return (DigiflipEvolutionContext){
        .care_mistakes = game->care_mistakes,
        .trainings = game->trainings,
        .overfeeds = game->overfeeds,
        .battles = game->battles,
        .victories = digiflip_game_recent_victories(game),
        .egg = (DigiflipEgg)game->egg_id,
        .tag_partner = game->tag_partner,
        .tag_battles = game->tag_battles,
        .elapsed_seconds = game->stage_elapsed,
    };
}

uint8_t digiflip_stage_v_chance(uint8_t victories, bool traited) {
    if(victories < DIGIFLIP_STAGE_V_CHANCE_MIN_WINS) return 0;
    if(victories >= DIGIFLIP_STAGE_V_SURE_WINS) return 100;
    const uint8_t chance = (uint8_t)((victories - DIGIFLIP_STAGE_V_CHANCE_MIN_WINS + 1U) *
                                         DIGIFLIP_STAGE_V_CHANCE_PER_WIN +
                                     (traited ? DIGIFLIP_TRAIT_BONUS_PCT : 0U));
    return chance > 100U ? 100U : chance;
}

/* A Stage IV Digimon that missed its Stage V route only on victories may
   still take it by chance. Returns the route, and the chance rolled in
   `chance` (0 when no roll happened). */
static const DigiflipEvolutionRule*
    digiflip_stage_v_roll(DigiflipGame* game, DigiflipEvolutionContext context, uint8_t* chance) {
    *chance = 0;
    if(game->stage != DigiflipStageAdult) return NULL;
    const uint8_t odds = digiflip_stage_v_chance((uint8_t)context.victories, game->traited);
    if(odds == 0 || odds == 100) return NULL;
    context.victories = DIGIFLIP_STAGE_V_SURE_WINS;
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(game->species_id, &context);
    if(!rule) return NULL;
    *chance = odds;
    return digiflip_rng_next(game) % 100U < odds ? rule : NULL;
}

bool digiflip_game_try_evolve(DigiflipGame* game) {
    if(!game->alive) return false;

    if(game->stage == DigiflipStageEgg) {
        const uint32_t duration = digiflip_stage_duration(DigiflipStageEgg);
        if(game->stage_elapsed < duration) return false;
        const uint32_t overflow = game->stage_elapsed - duration;
        game->evo_from_species = DIGIFLIP_SPECIES_NONE; /* hatched from egg */
        digiflip_game_set_species(game, digiflip_eggs[game->egg_id].hatch_species, overflow);
        game->hunger = 0;
        game->strength = 0;
        game->evo_pending = true;
        digiflip_emit(game, DigiflipEventHatched, 0, game->species_id, 0);
        return true;
    }

    const DigiflipStage stage = (DigiflipStage)game->stage;
    const uint32_t duration = digiflip_stage_duration(stage);
    const bool fusion_stage = stage == DigiflipStageUltimate;
    if((!fusion_stage && (duration == 0 || game->stage_elapsed < duration)) ||
       game->evolution_attempted)
        return false;

    const DigiflipEvolutionContext context = digiflip_game_evolution_context(game);
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(game->species_id, &context);
    uint8_t chance = 0;
    if(!rule && !fusion_stage) rule = digiflip_stage_v_roll(game, context, &chance);
    if(!rule) {
        if(!fusion_stage) {
            game->evolution_attempted = true;
            digiflip_emit(game, DigiflipEventNoEvolution, chance, game->species_id, 0);
        }
        return false;
    }

    const uint32_t overflow =
        duration && game->stage_elapsed > duration ? game->stage_elapsed - duration : 0;
    game->evo_from_species = game->species_id;
    digiflip_game_set_species(game, rule->to, overflow);
    game->evo_pending = true;
    digiflip_emit(game, DigiflipEventEvolved, chance, game->evo_from_species, game->species_id);
    return true;
}

static uint32_t digiflip_next_7am(uint32_t now);

/* Falls asleep for the night on its own, leaving the lights on. */
static void digiflip_fall_asleep(DigiflipGame* game, uint32_t now) {
    game->sleep_started_at = now;
    game->sleep_wake_at = digiflip_next_7am(now);
    game->tired = false;
    game->call_reason = DigiflipCallNone;
    game->call_latch = 0;
    game->call_deadline = 0;
    digiflip_emit(game, DigiflipEventSlept, 1U, 1U, 0);
}

/* Call-state machine: one active call at a time, with a 10-minute deadline
   for hunger/strength and 30 minutes for tired. No repeat call for a
   condition until it clears (latch). If both meters are empty when a call
   expires, that counts as a single care mistake. */
static void digiflip_game_update_call(DigiflipGame* game, uint32_t now) {
    if(!game->alive) return;

    if(game->hunger > 0) game->call_latch &= (uint8_t)~DigiflipCallHunger;
    if(game->strength > 0) game->call_latch &= (uint8_t)~DigiflipCallStrength;
    if(!game->tired) game->call_latch &= (uint8_t)~DigiflipCallTired;

    if(game->call_reason != DigiflipCallNone) {
        bool resolved = false;
        switch((DigiflipCallReason)game->call_reason) {
        case DigiflipCallHunger:
            resolved = game->hunger > 0;
            break;
        case DigiflipCallStrength:
            resolved = game->strength > 0;
            break;
        case DigiflipCallTired:
            resolved = !game->tired;
            break;
        default:
            break;
        }
        if(resolved) {
            digiflip_emit(game, DigiflipEventCallAnswered, game->call_reason, 0, 0);
            game->call_reason = DigiflipCallNone;
        } else if(now >= game->call_deadline) {
            const bool bedtime_missed = game->call_reason == DigiflipCallTired;
            /* Care-mistake tracking starts at Baby II; the hatch call during
               Baby I still alerts but never counts. */
            if(game->stage >= DigiflipStageBabyII) {
                if(game->care_mistakes < UINT16_MAX) game->care_mistakes++;
                digiflip_emit(
                    game, DigiflipEventCareMistake, game->call_reason, game->care_mistakes, 0);
                digiflip_game_check_mistake_deaths(game);
            }
            /* One mistake per neglect episode: latch any other empty meter. */
            if(game->hunger == 0) game->call_latch |= DigiflipCallHunger;
            if(game->strength == 0) game->call_latch |= DigiflipCallStrength;
            game->call_reason = DigiflipCallNone;
            if(!game->alive) return;
            if(bedtime_missed) {
                /* Nobody put it to bed: it nods off anyway, lights still on. */
                digiflip_fall_asleep(game, now);
                return;
            }
        } else {
            return;
        }
    }

    if(digiflip_is_asleep(game) || game->stage == DigiflipStageEgg) return;

    /* Priority: tired first; while tired, empty meters don't call. */
    if(game->tired && !(game->call_latch & DigiflipCallTired)) {
        game->call_reason = DigiflipCallTired;
        game->call_deadline = digiflip_sat_add32(now, DIGIFLIP_TIRED_CALL_TIMEOUT_SECONDS);
        game->call_latch |= DigiflipCallTired;
        digiflip_emit(game, DigiflipEventCall, DigiflipCallTired, 0, 0);
    } else if(!game->tired && game->hunger == 0 && !(game->call_latch & DigiflipCallHunger)) {
        game->call_reason = DigiflipCallHunger;
        game->call_deadline = digiflip_sat_add32(now, DIGIFLIP_CALL_TIMEOUT_SECONDS);
        game->call_latch |= DigiflipCallHunger;
        digiflip_emit(game, DigiflipEventCall, DigiflipCallHunger, 0, 0);
    } else if(!game->tired && game->strength == 0 && !(game->call_latch & DigiflipCallStrength)) {
        game->call_reason = DigiflipCallStrength;
        game->call_deadline = digiflip_sat_add32(now, DIGIFLIP_CALL_TIMEOUT_SECONDS);
        game->call_latch |= DigiflipCallStrength;
        digiflip_emit(game, DigiflipEventCall, DigiflipCallStrength, 0, 0);
    }
}

static uint32_t digiflip_next_7am(uint32_t now) {
    const uint32_t wake = (now / 86400U) * 86400U + 7U * 3600U;
    return wake > now ? wake : wake + 86400U;
}

/* Next bedtime strictly after `now`, skipping days already triggered. */
static uint32_t digiflip_next_bedtime_after(const DigiflipGame* game, uint32_t now) {
    const DigiflipSpecies* species = digiflip_species_get(game->species_id);
    if(!species || species->sleep_minutes == DIGIFLIP_NO_SLEEP) return UINT32_MAX;
    const uint32_t bedtime = (uint32_t)species->sleep_minutes * 60U;
    uint32_t day = now / 86400U;
    if(now % 86400U >= bedtime) day++;
    if(game->tired_day != UINT32_MAX && day <= game->tired_day) day = game->tired_day + 1;
    return day * 86400U + bedtime;
}

/* Absolute time of the next evolution attempt, or UINT32_MAX if none is
   scheduled (attempted already, or fusion stage which evolves via battles). */
static uint32_t digiflip_evolution_at(const DigiflipGame* game, uint32_t now) {
    if(game->stage == DigiflipStageEgg) {
        if(game->stage_elapsed >= digiflip_stage_duration(DigiflipStageEgg)) return now;
        return digiflip_sat_add32(
            now, digiflip_stage_duration(DigiflipStageEgg) - game->stage_elapsed);
    }
    if(game->evolution_attempted) return UINT32_MAX;
    const uint32_t duration = digiflip_stage_duration((DigiflipStage)game->stage);
    if(duration == 0) return UINT32_MAX;
    if(game->stage_elapsed >= duration) return now;
    return digiflip_sat_add32(now, duration - game->stage_elapsed);
}

static void digiflip_fire_due_events(DigiflipGame* game, uint32_t now) {
    if(!game->alive) return;
    bool asleep = digiflip_is_asleep(game);
    if(asleep && now >= game->sleep_wake_at) {
        digiflip_game_wake(game, now);
        asleep = false;
    }

    if(!asleep && game->stage != DigiflipStageEgg) {
        if(game->next_heart_at != 0 && now >= game->next_heart_at) {
            /* The "no heart loss" cheat keeps the clock ticking but skips the drop. */
            if(game->hunger > 0 && !digiflip_rules.freeze_hearts) {
                game->hunger--;
                if(game->hunger == 0) {
                    game->hunger_empty_since = now;
                    digiflip_emit(game, DigiflipEventHungerEmpty, 0, 0, 0);
                }
            }
            if(game->strength > 0 && !digiflip_rules.freeze_hearts) {
                game->strength--;
                if(game->strength == 0) {
                    game->strength_empty_since = now;
                    digiflip_emit(game, DigiflipEventStrengthEmpty, 0, 0, 0);
                }
            }
            game->overfeed_counted = false;
            game->next_heart_at = digiflip_sat_add32(now, digiflip_game_heart_interval(game, now));
        }

        if(game->next_poop_at != 0 && now >= game->next_poop_at) {
            if(game->messes < 4 && !digiflip_rules.no_waste) {
                game->messes++;
                digiflip_emit(game, DigiflipEventPooped, game->messes, 0, 0);
                /* 4 piles of waste injures; no repeat injury while injured. */
                if(game->messes >= 4 && !game->injured) {
                    digiflip_game_injure(game, now, DigiflipInjuryWaste);
                }
            }
            game->next_poop_at = digiflip_sat_add32(now, digiflip_game_poop_interval(game, now));
        }

        if(!game->tired) {
            const DigiflipSpecies* species = digiflip_species_get(game->species_id);
            if(species && species->sleep_minutes != DIGIFLIP_NO_SLEEP &&
               game->tired_day != now / 86400U &&
               now % 86400U >= (uint32_t)species->sleep_minutes * 60U) {
                game->tired = true;
                game->tired_day = now / 86400U;
                digiflip_emit(game, DigiflipEventTired, 0, 0, 0);
            }
        }

        for(uint8_t i = 0; i < 7 && digiflip_game_try_evolve(game); i++) {
        }
    } else if(!asleep) {
        for(uint8_t i = 0; i < 7 && digiflip_game_try_evolve(game); i++) {
        }
    }

    if(!game->alive) return;

    /* Deaths from unattended conditions run on wall-clock time, asleep or not. */
    if(game->injured && now - game->injured_at >= DIGIFLIP_INJURY_DEATH_SECONDS) {
        digiflip_game_kill(game, DigiflipDeathInjuredTooLong);
        return;
    }
    if(game->stage != DigiflipStageEgg) {
        if(game->hunger == 0 && game->hunger_empty_since != 0 &&
           now - game->hunger_empty_since >= DIGIFLIP_STARVE_DEATH_SECONDS) {
            digiflip_game_kill(game, DigiflipDeathStarved);
            return;
        }
        if(game->strength == 0 && game->strength_empty_since != 0 &&
           now - game->strength_empty_since >= DIGIFLIP_STARVE_DEATH_SECONDS) {
            digiflip_game_kill(game, DigiflipDeathStarved);
            return;
        }
    }

    digiflip_game_update_call(game, now);
}

/* Time-sliced simulation: jump from event to event so everything that
   happened offline fires in chronological order. */
static void digiflip_game_simulate(DigiflipGame* game, uint32_t from, uint32_t to) {
    uint32_t now = from;
    for(uint32_t i = 0; i < (1U << 20) && now < to && game->alive; i++) {
        const bool asleep = digiflip_is_asleep(game);
        if(!asleep && game->stage != DigiflipStageEgg) {
            /* Repair clocks left unscheduled by evolution, wake, or hatch.
               Strictly less-than: an event exactly due must fire, not slip. */
            if(game->next_heart_at < now)
                game->next_heart_at =
                    digiflip_sat_add32(now, digiflip_game_heart_interval(game, now));
            if(game->next_poop_at < now)
                game->next_poop_at =
                    digiflip_sat_add32(now, digiflip_game_poop_interval(game, now));
        }

        digiflip_replay_now = now;
        digiflip_fire_due_events(game, now);
        if(!game->alive || now >= to) break;

        uint32_t next = to;
        const bool still_asleep = digiflip_is_asleep(game);
        if(!still_asleep && game->stage != DigiflipStageEgg) {
            if(game->next_heart_at > now && game->next_heart_at < next) next = game->next_heart_at;
            if(game->next_poop_at > now && game->next_poop_at < next) next = game->next_poop_at;
            const uint32_t bedtime = digiflip_next_bedtime_after(game, now);
            if(bedtime < next) next = bedtime;
            const uint32_t evolution = digiflip_evolution_at(game, now);
            if(evolution > now && evolution < next) next = evolution;
        } else if(!still_asleep) {
            const uint32_t evolution = digiflip_evolution_at(game, now);
            if(evolution > now && evolution < next) next = evolution;
        }
        if(still_asleep && game->sleep_wake_at > now && game->sleep_wake_at < next)
            next = game->sleep_wake_at;
        if(game->call_reason != DigiflipCallNone && game->call_deadline > now &&
           game->call_deadline < next)
            next = game->call_deadline;
        if(game->injured) {
            const uint32_t deadline =
                digiflip_sat_add32(game->injured_at, DIGIFLIP_INJURY_DEATH_SECONDS);
            if(deadline > now && deadline < next) next = deadline;
        }
        if(game->hunger == 0 && game->hunger_empty_since != 0) {
            const uint32_t deadline =
                digiflip_sat_add32(game->hunger_empty_since, DIGIFLIP_STARVE_DEATH_SECONDS);
            if(deadline > now && deadline < next) next = deadline;
        }
        if(game->strength == 0 && game->strength_empty_since != 0) {
            const uint32_t deadline =
                digiflip_sat_add32(game->strength_empty_since, DIGIFLIP_STARVE_DEATH_SECONDS);
            if(deadline > now && deadline < next) next = deadline;
        }

        /* Continuous clocks run only while awake (frozen during sleep). */
        if(!still_asleep && next > now) {
            const uint32_t dt = next - now;
            game->stage_elapsed = digiflip_sat_add32(game->stage_elapsed, dt);
            game->awake_seconds = digiflip_sat_add32(game->awake_seconds, dt);
        }
        now = next;
    }
    /* Fire anything due exactly at `to` so a single tick is exact. */
    digiflip_replay_now = to;
    if(game->alive) digiflip_fire_due_events(game, to);
}

void digiflip_game_tick(DigiflipGame* game, uint32_t now) {
    if(!game->alive) return;
    if(now < game->last_update_at) {
        game->last_update_at = now;
        return;
    }
    if(now == game->last_update_at) return;

    /* Meters already empty at an unknown time: assume they emptied at the
       last update so offline deaths are never backdated unfairly. */
    if(game->hunger == 0 && game->hunger_empty_since == 0)
        game->hunger_empty_since = game->last_update_at;
    if(game->strength == 0 && game->strength_empty_since == 0)
        game->strength_empty_since = game->last_update_at;

    digiflip_replaying = true;
    digiflip_game_simulate(game, game->last_update_at, now);
    digiflip_replaying = false;
    game->last_update_at = now;
}

bool digiflip_game_feed_meat(DigiflipGame* game) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    bool overfed = false;
    if(game->hunger < DIGIFLIP_MAX_HEARTS) {
        game->hunger++;
    } else if(!game->overfeed_counted) {
        game->overfeeds++;
        game->overfeed_counted = true;
        overfed = true;
    } else {
        return false;
    }
    digiflip_emit(game, DigiflipEventFed, DigiflipFoodMeat, overfed ? 1U : 0U, 0);
    if(game->hunger > 0) game->hunger_empty_since = 0;
    if(game->weight < 99) game->weight++;
    return true;
}

bool digiflip_game_food_fills(const DigiflipGame* game, DigiflipFood food) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    const uint8_t hearts = food == DigiflipFoodMeat ? game->hunger : game->strength;
    return hearts < DIGIFLIP_MAX_HEARTS;
}

bool digiflip_game_feed_protein(DigiflipGame* game) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    if(game->strength < DIGIFLIP_MAX_HEARTS) game->strength++;
    if(game->strength > 0) game->strength_empty_since = 0;
    const uint16_t max = digiflip_game_dp_max(game);
    if(game->dp_quarters < max) game->dp_quarters++;
    game->weight = game->weight <= 97 ? (uint8_t)(game->weight + 2U) : 99U;
    digiflip_emit(game, DigiflipEventFed, DigiflipFoodProtein, 0, 0);
    return true;
}

/* Shared by single and tag training: effort always, strength on success. */
static bool digiflip_train_outcome(DigiflipGame* game, bool success, uint16_t detail, bool tag) {
    if(game->trainings < UINT16_MAX) game->trainings++;
    game->effort = (uint8_t)(game->trainings / 4U);
    if(game->effort > DIGIFLIP_MAX_HEARTS) game->effort = DIGIFLIP_MAX_HEARTS;
    if(success) {
        game->strength = (uint8_t)(game->strength + DIGIFLIP_TRAIN_STRENGTH_GAIN);
        if(game->strength > DIGIFLIP_MAX_HEARTS) game->strength = DIGIFLIP_MAX_HEARTS;
        game->strength_empty_since = 0;
        const uint8_t shed = (uint8_t)(1U + digiflip_rng_next(game) % 4U);
        game->weight = game->weight > shed ? (uint8_t)(game->weight - shed) : 1U;
    }
    digiflip_emit(game, DigiflipEventTrained, success ? 1U : 0U, detail, tag ? 1U : 0U);
    return success;
}

bool digiflip_game_train(DigiflipGame* game, uint16_t presses) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    return digiflip_train_outcome(game, presses >= DIGIFLIP_TRAIN_PRESSES_NEEDED, presses, false);
}

bool digiflip_game_tag_guard_high(DigiflipGame* game) {
    return (digiflip_rng_next(game) & 1U) != 0;
}

bool digiflip_game_tag_train(DigiflipGame* game, uint8_t hits) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    return digiflip_train_outcome(game, hits >= DIGIFLIP_TAG_TRAIN_NEEDED, hits, true);
}

bool digiflip_game_clean(DigiflipGame* game) {
    if(!game->alive || game->messes == 0) return false;
    digiflip_emit(game, DigiflipEventCleaned, game->messes, 0, 0);
    game->messes = 0;
    return true;
}

bool digiflip_game_sleep(DigiflipGame* game, uint32_t now) {
    if(!game->alive || game->stage == DigiflipStageEgg || digiflip_is_asleep(game)) return false;
    /* Caller ticks first; `now` must be current. */
    game->sleep_started_at = now;
    if(game->tired) {
        game->sleep_wake_at = digiflip_next_7am(now);
    } else {
        const DigiflipSpecies* species = digiflip_species_get(game->species_id);
        const uint32_t nap_wake = digiflip_sat_add32(now, DIGIFLIP_NAP_SECONDS);
        uint32_t wake = nap_wake;
        if(species && species->sleep_minutes != DIGIFLIP_NO_SLEEP) {
            const uint32_t bedtime =
                (now / 86400U) * 86400U + (uint32_t)species->sleep_minutes * 60U;
            /* A nap that would cross bedtime becomes night sleep. */
            if(nap_wake >= bedtime) wake = digiflip_next_7am(now);
        }
        game->sleep_wake_at = wake;
    }
    game->tired = false;
    game->lights_on = false;
    /* Night sleep runs to 7:00 AM; anything shorter is a nap. */
    digiflip_emit(
        game, DigiflipEventSlept, game->sleep_wake_at - now > DIGIFLIP_NAP_SECONDS ? 1U : 0U, 0, 0);
    /* Falling asleep clears any active call: there is no call light while
       asleep, so no mistake accrues for it. Meter calls resume fresh on
       wake (latches cleared here). */
    game->call_reason = DigiflipCallNone;
    game->call_latch = 0;
    game->call_deadline = 0;
    return true;
}

bool digiflip_game_wake(DigiflipGame* game, uint32_t now) {
    if(!digiflip_is_asleep(game)) return false;
    digiflip_emit(game, DigiflipEventWoke, now >= game->sleep_wake_at ? 1U : 0U, 0, 0);
    const uint32_t slept = now >= game->sleep_started_at ? now - game->sleep_started_at : 0;
    if(slept >= DIGIFLIP_NAP_SECONDS) game->dp_quarters = digiflip_game_dp_max(game);
    game->sleep_started_at = 0;
    game->sleep_wake_at = 0;
    game->lights_on = true;
    game->next_heart_at = digiflip_sat_add32(now, digiflip_game_heart_interval(game, now));
    game->next_poop_at = digiflip_sat_add32(now, digiflip_game_poop_interval(game, now));
    digiflip_game_update_call(game, now);
    return true;
}

bool digiflip_game_is_asleep(const DigiflipGame* game) {
    return digiflip_is_asleep(game);
}

bool digiflip_game_heal(DigiflipGame* game) {
    if(!game->alive || game->stage == DigiflipStageEgg || !game->injured) return false;
    /* Healing can take multiple doses. */
    const bool healed = (digiflip_rng_next(game) % 2U) == 0;
    if(healed) game->injured = false;
    digiflip_emit(game, DigiflipEventMedicine, healed ? 1U : 0U, 0, 0);
    return healed;
}

bool digiflip_game_is_injured(const DigiflipGame* game) {
    return game->alive && game->injured;
}

bool digiflip_game_is_tired(const DigiflipGame* game) {
    return game->alive && game->tired;
}

DigiflipCallReason digiflip_game_call_reason(const DigiflipGame* game) {
    return (DigiflipCallReason)game->call_reason;
}

uint16_t digiflip_game_dp_quarters(const DigiflipGame* game) {
    return game->dp_quarters;
}

uint8_t digiflip_game_injuries(const DigiflipGame* game) {
    return game->injuries;
}

DigiflipDeathCause digiflip_game_death_cause(const DigiflipGame* game) {
    return (DigiflipDeathCause)game->death_cause;
}

const char* digiflip_game_death_cause_name(DigiflipDeathCause cause) {
    switch(cause) {
    case DigiflipDeathInjuries:
        return "20 injuries";
    case DigiflipDeathCareMistakes:
        return "20 care mistakes";
    case DigiflipDeathNeglect:
        return "neglect";
    case DigiflipDeathInjuredTooLong:
        return "injured 6h";
    case DigiflipDeathStarved:
        return "starved 12h";
    case DigiflipDeathNone:
    default:
        return "";
    }
}

bool digiflip_game_record_battle(DigiflipGame* game, bool victory, DigiflipSpeciesId tag_partner) {
    if(!game->alive || game->stage < DigiflipStageChild) return false;
    if(game->battles < UINT16_MAX) game->battles++;
    if(game->total_battles < UINT16_MAX) game->total_battles++;
    if(victory && game->total_victories < UINT16_MAX) game->total_victories++;
    game->battle_history = (uint16_t)((game->battle_history << 1U) | (victory ? 1U : 0U));
    if(game->battle_history_count < 15) game->battle_history_count++;
    if(game->weight > 1) game->weight--;

    if(tag_partner != DIGIFLIP_SPECIES_NONE) {
        if(game->tag_partner == tag_partner) {
            if(game->tag_battles < UINT16_MAX) game->tag_battles++;
        } else {
            game->tag_partner = tag_partner;
            game->tag_battles = 1;
        }
    }
    return digiflip_game_try_evolve(game);
}

DigiflipBattleBlock digiflip_game_battle_block(const DigiflipGame* game) {
    if(!game->alive || game->stage < DigiflipStageChild) return DigiflipBattleTooYoung;
    if(digiflip_is_asleep(game)) return DigiflipBattleAsleep;
    if(game->injured) return DigiflipBattleInjured;
    if(game->dp_quarters < DIGIFLIP_BATTLE_DP_QUARTERS) return DigiflipBattleNoDp;
    return DigiflipBattleOk;
}

uint16_t digiflip_game_power(const DigiflipGame* game) {
    const DigiflipSpecies* species = digiflip_species_get(game->species_id);
    if(!species) return 0;
    uint16_t bonus = 0;
    if(game->weight < DIGIFLIP_MAX_WEIGHT) {
        bonus = (uint16_t)(game->strength * DIGIFLIP_STRENGTH_POWER_PER_HEART);
        if(bonus > DIGIFLIP_STRENGTH_POWER_MAX) bonus = DIGIFLIP_STRENGTH_POWER_MAX;
    }
    return (uint16_t)(species->power + bonus);
}

DigiflipAttack digiflip_battle_attack_for_presses(uint16_t presses) {
    const uint32_t rank =
        (uint32_t)presses * DigiflipAttackCritical / DIGIFLIP_BATTLE_CHARGE_PRESSES;
    return rank >= DigiflipAttackCritical ? DigiflipAttackCritical : (DigiflipAttack)rank;
}

const char* digiflip_battle_attack_name(DigiflipAttack attack) {
    switch(attack) {
    case DigiflipAttackStrong:
        return "Strong";
    case DigiflipAttackDoubleWeak:
        return "Double";
    case DigiflipAttackDoubleStrong:
        return "Double Strong";
    case DigiflipAttackCritical:
        return "Critical!";
    case DigiflipAttackWeak:
    default:
        return "Weak";
    }
}

static uint8_t digiflip_attack_damage(DigiflipAttack attack) {
    static const uint8_t damage[] = {3U, 4U, 5U, 6U, 7U};
    return attack <= DigiflipAttackCritical ? damage[attack] : damage[0];
}

const DigiflipColosseumOpponent* digiflip_game_next_opponent(const DigiflipGame* game) {
    return digiflip_colosseum_opponent((uint8_t)(game->battle_round % DIGIFLIP_COLOSSEUM_ROUNDS));
}

uint8_t digiflip_game_win_percent(const DigiflipGame* game) {
    if(game->total_battles == 0) return 0;
    return (uint8_t)((uint32_t)game->total_victories * 100U / game->total_battles);
}

bool digiflip_game_battle(
    DigiflipGame* game,
    uint16_t charge_presses,
    DigiflipBattleResult* result) {
    DigiflipBattleResult scratch;
    if(!result) result = &scratch;
    memset(result, 0, sizeof(*result));
    result->player_species = game->species_id;
    if(digiflip_game_battle_block(game) != DigiflipBattleOk) return false;
    const DigiflipSpecies* species = digiflip_species_get(game->species_id);
    const DigiflipColosseumOpponent* opponent = digiflip_game_next_opponent(game);
    if(!species || !opponent) return false;
    game->dp_quarters = (uint16_t)(game->dp_quarters - DIGIFLIP_BATTLE_DP_QUARTERS);

    result->round = (uint8_t)(game->battle_round % DIGIFLIP_COLOSSEUM_ROUNDS);
    result->opponent = opponent;
    result->attack = digiflip_battle_attack_for_presses(charge_presses);

    /* Hit rate: P * 100 / (P + E), with the attribute advantage folded into
       power. */
    uint32_t player_power = digiflip_game_power(game);
    uint32_t enemy_power = opponent->power;
    if(digiflip_attribute_advantage(species->attribute, opponent->attribute))
        player_power += DIGIFLIP_ATTRIBUTE_BONUS;
    if(digiflip_attribute_advantage(opponent->attribute, species->attribute))
        enemy_power += DIGIFLIP_ATTRIBUTE_BONUS;
    uint32_t hit_rate =
        player_power + enemy_power ? player_power * 100U / (player_power + enemy_power) : 50U;
    if(hit_rate < 1U) hit_rate = 1U;
    if(hit_rate > 99U) hit_rate = 99U;
    result->hit_rate = (uint8_t)hit_rate;

    uint8_t player_damage = digiflip_attack_damage(result->attack);
    if(game->strength >= DIGIFLIP_MAX_HEARTS && game->weight < DIGIFLIP_MAX_WEIGHT)
        player_damage = (uint8_t)(player_damage + DIGIFLIP_BATTLE_STRENGTH_DAMAGE);

    uint8_t player_hp = DIGIFLIP_BATTLE_HP;
    uint8_t enemy_hp = DIGIFLIP_BATTLE_HP;
    for(uint8_t i = 0; i < DIGIFLIP_BATTLE_MAX_EXCHANGES && player_hp && enemy_hp; i++) {
        DigiflipBattleExchange* exchange = &result->exchanges[i];
        exchange->by_player = (i & 1U) == 0;
        exchange->attacker = exchange->by_player ? DigiflipFighterPet : DigiflipFighterLeft;
        const uint32_t roll = digiflip_rng_next(game) % 100U;
        exchange->hit = exchange->by_player ? roll < hit_rate : roll >= hit_rate;
        if(exchange->hit) {
            exchange->damage = exchange->by_player ? player_damage : DIGIFLIP_BATTLE_ENEMY_DAMAGE;
            uint8_t* hp = exchange->by_player ? &enemy_hp : &player_hp;
            *hp = *hp > exchange->damage ? (uint8_t)(*hp - exchange->damage) : 0U;
        }
        result->exchange_count = (uint8_t)(i + 1U);
    }
    /* A drawn-out fight goes to whoever has more HP left; ties go to the house. */
    result->victory = enemy_hp == 0 || (player_hp > 0 && player_hp > enemy_hp);

    digiflip_emit(game, DigiflipEventBattle, result->victory ? 1U : 0U, result->round + 1U, 0);
    if(result->victory) {
        game->battle_round = (uint8_t)((result->round + 1U) % DIGIFLIP_COLOSSEUM_ROUNDS);
    }
    result->evolved = digiflip_game_record_battle(game, result->victory, DIGIFLIP_SPECIES_NONE);

    /* A battle that triggers evolution leaves the new form unhurt. */
    const uint32_t chance = result->victory ? DIGIFLIP_INJURY_CHANCE_WIN_PCT :
                                              DIGIFLIP_INJURY_CHANCE_LOSS_PCT;
    if(!result->evolved && (digiflip_rng_next(game) % 100U) < chance) {
        digiflip_game_injure(game, game->last_update_at, DigiflipInjuryBattle);
        result->injured = true;
    }
    return true;
}

uint16_t digiflip_game_age_days(const DigiflipGame* game, uint32_t now) {
    if(now < game->born_at) return 0;
    const uint32_t days = (now - game->born_at) / (24U * 60U * 60U);
    return days > UINT16_MAX ? UINT16_MAX : (uint16_t)days;
}

const char* digiflip_game_stage_name(const DigiflipGame* game) {
    return digiflip_stage_name((DigiflipStage)game->stage);
}

const char* digiflip_game_species_name(const DigiflipGame* game) {
    const DigiflipSpecies* species = digiflip_species_get(game->species_id);
    return species ? species->name : "Digi-Egg";
}

bool digiflip_game_evo_pending(const DigiflipGame* game) {
    return game->evo_pending;
}

DigiflipSpeciesId digiflip_game_evo_from_species(const DigiflipGame* game) {
    return game->evo_from_species;
}

void digiflip_game_clear_evo_pending(DigiflipGame* game) {
    game->evo_pending = false;
}

void digiflip_game_fill_hearts(DigiflipGame* game) {
    if(!game->alive || game->stage == DigiflipStageEgg) return;
    game->hunger = DIGIFLIP_MAX_HEARTS;
    game->strength = DIGIFLIP_MAX_HEARTS;
    game->hunger_empty_since = 0;
    game->strength_empty_since = 0;
}

bool digiflip_game_force_evolve(DigiflipGame* game) {
    if(!game->alive) return false;
    if(game->stage == DigiflipStageEgg) {
        game->stage_elapsed = digiflip_stage_duration(DigiflipStageEgg);
        return digiflip_game_try_evolve(game);
    }
    /* The route these stats qualify for (ignoring the stage timer), or the
       species' first route if none do. Tag-partner requirements are waived. */
    DigiflipEvolutionContext context = digiflip_game_evolution_context(game);
    context.elapsed_seconds = UINT32_MAX;
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(game->species_id, &context);
    for(size_t i = 0; !rule && i < digiflip_evolution_rule_count; i++) {
        if(digiflip_evolution_rules[i].from == game->species_id)
            rule = &digiflip_evolution_rules[i];
    }
    if(!rule) return false; /* a final form */
    game->evo_from_species = game->species_id;
    digiflip_game_set_species(game, rule->to, 0);
    game->evo_pending = true;
    digiflip_emit(game, DigiflipEventEvolved, 0, game->evo_from_species, game->species_id);
    return true;
}

/* Minutes between lost hearts and poops, by stage. */
static const uint16_t digiflip_heart_minutes[] = {
    [DigiflipStageEgg] = 30,
    [DigiflipStageBabyI] = 3,
    [DigiflipStageBabyII] = 15,
    [DigiflipStageChild] = 30,
    [DigiflipStageAdult] = 45,
    [DigiflipStagePerfect] = 60,
    [DigiflipStageUltimate] = 60,
    [DigiflipStageSuperUltimate] = 60,
};
static const uint16_t digiflip_poop_minutes[] = {
    [DigiflipStageEgg] = 30,
    [DigiflipStageBabyI] = 5,
    [DigiflipStageBabyII] = 20,
    [DigiflipStageChild] = 30,
    [DigiflipStageAdult] = 45,
    [DigiflipStagePerfect] = 60,
    [DigiflipStageUltimate] = 60,
    [DigiflipStageSuperUltimate] = 60,
};

static uint32_t
    digiflip_need_interval(const DigiflipGame* game, uint32_t now, const uint16_t* minutes) {
    const uint8_t stage = game->stage <= DigiflipStageSuperUltimate ? game->stage : 0U;
    uint32_t seconds = (uint32_t)minutes[stage] * 60U;
    /* Old Digimon need looking after more often again. */
    if(digiflip_game_age_days(game, now) >= DIGIFLIP_OLD_AGE_DAYS) seconds = seconds * 2U / 3U;
    return seconds;
}

uint32_t digiflip_game_heart_interval(const DigiflipGame* game, uint32_t now) {
    return digiflip_need_interval(game, now, digiflip_heart_minutes);
}

uint32_t digiflip_game_poop_interval(const DigiflipGame* game, uint32_t now) {
    return digiflip_need_interval(game, now, digiflip_poop_minutes);
}

bool digiflip_game_lights_off(DigiflipGame* game) {
    if(!game->alive || !digiflip_is_asleep(game) || !game->lights_on) return false;
    game->lights_on = false;
    digiflip_emit(game, DigiflipEventLightsOff, 0, 0, 0);
    return true;
}

const DigiflipTagRound* digiflip_game_next_tag_round(const DigiflipGame* game) {
    return digiflip_colosseum_tag_round((uint8_t)(game->tag_round % DIGIFLIP_COLOSSEUM_ROUNDS));
}

/* Hit chance for one attacker against one defender, with the attribute
   advantage folded into power, clamped to 1-99%. */
static uint8_t digiflip_hit_rate(
    uint32_t power,
    DigiflipAttribute attribute,
    uint32_t enemy_power,
    DigiflipAttribute enemy_attribute) {
    if(digiflip_attribute_advantage(attribute, enemy_attribute)) power += DIGIFLIP_ATTRIBUTE_BONUS;
    if(digiflip_attribute_advantage(enemy_attribute, attribute))
        enemy_power += DIGIFLIP_ATTRIBUTE_BONUS;
    uint32_t rate = power + enemy_power ? power * 100U / (power + enemy_power) : 50U;
    if(rate < 1U) rate = 1U;
    if(rate > 99U) rate = 99U;
    return (uint8_t)rate;
}

bool digiflip_game_tag_battle(
    DigiflipGame* game,
    DigiflipSpeciesId partner,
    uint16_t charge_presses,
    DigiflipSync sync,
    DigiflipBattleResult* result) {
    return digiflip_game_tag_battle_team(game, NULL, partner, charge_presses, sync, result);
}

bool digiflip_game_tag_battle_team(
    DigiflipGame* game,
    DigiflipGame* mate_game,
    DigiflipSpeciesId partner,
    uint16_t charge_presses,
    DigiflipSync sync,
    DigiflipBattleResult* result) {
    DigiflipBattleResult scratch;
    if(!result) result = &scratch;
    memset(result, 0, sizeof(*result));
    if(mate_game) partner = mate_game->species_id;
    result->tag = true;
    result->player_species = game->species_id;
    result->partner_species = partner;
    if(digiflip_game_battle_block(game) != DigiflipBattleOk) return false;
    if(mate_game && digiflip_game_battle_block(mate_game) != DigiflipBattleOk) return false;
    const DigiflipSpecies* pet = digiflip_species_get(game->species_id);
    const DigiflipSpecies* mate = digiflip_species_get(partner);
    const DigiflipTagRound* round = digiflip_game_next_tag_round(game);
    if(!pet || !mate || !round) return false;
    game->dp_quarters = (uint16_t)(game->dp_quarters - DIGIFLIP_BATTLE_DP_QUARTERS);
    if(mate_game) {
        mate_game->dp_quarters = (uint16_t)(mate_game->dp_quarters - DIGIFLIP_BATTLE_DP_QUARTERS);
    }

    result->round = (uint8_t)(game->tag_round % DIGIFLIP_COLOSSEUM_ROUNDS);
    result->opponent = &round->left;
    result->opponent2 = &round->right;
    int rank = (int)digiflip_battle_attack_for_presses(charge_presses);
    rank += sync == DigiflipSyncPerfect ? 1 : (sync == DigiflipSyncMiss ? -1 : 0);
    if(rank < (int)DigiflipAttackWeak) rank = DigiflipAttackWeak;
    if(rank > (int)DigiflipAttackCritical) rank = DigiflipAttackCritical;
    result->attack = (DigiflipAttack)rank;

    /* Lanes: the pet trades blows with the left opponent, its teammate with
       the right. A Copymon always fights at full strength; a raised teammate
       brings its own hearts and weight. */
    const uint32_t pet_power = digiflip_game_power(game);
    const uint32_t mate_power = mate_game ? digiflip_game_power(mate_game) :
                                            (uint32_t)mate->power + DIGIFLIP_STRENGTH_POWER_MAX;
    const uint8_t rates[4] = {
        [DigiflipFighterPet] =
            digiflip_hit_rate(pet_power, pet->attribute, round->left.power, round->left.attribute),
        [DigiflipFighterPartner] = digiflip_hit_rate(
            mate_power, mate->attribute, round->right.power, round->right.attribute),
        [DigiflipFighterLeft] =
            digiflip_hit_rate(round->left.power, round->left.attribute, pet_power, pet->attribute),
        [DigiflipFighterRight] = digiflip_hit_rate(
            round->right.power, round->right.attribute, mate_power, mate->attribute),
    };
    result->hit_rate = rates[DigiflipFighterPet];

    const uint8_t base = digiflip_attack_damage(result->attack);
    const bool pet_strong = game->strength >= DIGIFLIP_MAX_HEARTS &&
                            game->weight < DIGIFLIP_MAX_WEIGHT;
    const bool mate_strong = !mate_game || (mate_game->strength >= DIGIFLIP_MAX_HEARTS &&
                                            mate_game->weight < DIGIFLIP_MAX_WEIGHT);
    const uint8_t damage[4] = {
        [DigiflipFighterPet] = (uint8_t)(base + (pet_strong ? DIGIFLIP_TAG_STRENGTH_DAMAGE : 0U)),
        [DigiflipFighterPartner] =
            (uint8_t)(base + (mate_strong ? DIGIFLIP_TAG_STRENGTH_DAMAGE : 0U)),
        [DigiflipFighterLeft] = DIGIFLIP_BATTLE_ENEMY_DAMAGE,
        [DigiflipFighterRight] = DIGIFLIP_BATTLE_ENEMY_DAMAGE,
    };
    static const uint8_t order[4] = {
        DigiflipFighterPet, DigiflipFighterLeft, DigiflipFighterPartner, DigiflipFighterRight};

    uint8_t team_hp = DIGIFLIP_TAG_BATTLE_HP;
    uint8_t enemy_hp = DIGIFLIP_TAG_BATTLE_HP;
    for(uint8_t i = 0; i < DIGIFLIP_BATTLE_MAX_EXCHANGES && team_hp && enemy_hp; i++) {
        DigiflipBattleExchange* exchange = &result->exchanges[i];
        exchange->attacker = order[i % 4U];
        exchange->by_player = exchange->attacker == DigiflipFighterPet ||
                              exchange->attacker == DigiflipFighterPartner;
        exchange->hit = (digiflip_rng_next(game) % 100U) < rates[exchange->attacker];
        if(exchange->hit) {
            exchange->damage = damage[exchange->attacker];
            uint8_t* hp = exchange->by_player ? &enemy_hp : &team_hp;
            *hp = *hp > exchange->damage ? (uint8_t)(*hp - exchange->damage) : 0U;
        }
        result->exchange_count = (uint8_t)(i + 1U);
    }
    result->victory = enemy_hp == 0 || (team_hp > 0 && team_hp > enemy_hp);

    digiflip_emit(game, DigiflipEventBattle, result->victory ? 1U : 0U, result->round + 1U, 1U);
    if(mate_game) {
        digiflip_emit(
            mate_game, DigiflipEventBattle, result->victory ? 1U : 0U, result->round + 1U, 1U);
    }
    if(result->victory) {
        game->tag_round = (uint8_t)((result->round + 1U) % DIGIFLIP_COLOSSEUM_ROUNDS);
    }
    const DigiflipSpeciesId lead_species = game->species_id;
    result->evolved = digiflip_game_record_battle(game, result->victory, partner);
    /* A raised teammate counts the battle too, with the lead as its partner. */
    if(mate_game) {
        result->mate_evolved =
            digiflip_game_record_battle(mate_game, result->victory, lead_species);
    }

    const uint32_t chance = result->victory ? DIGIFLIP_INJURY_CHANCE_WIN_PCT :
                                              DIGIFLIP_INJURY_CHANCE_LOSS_PCT;
    if(!result->evolved && (digiflip_rng_next(game) % 100U) < chance) {
        digiflip_game_injure(game, game->last_update_at, DigiflipInjuryBattle);
        result->injured = true;
    }
    if(mate_game && !result->mate_evolved && (digiflip_rng_next(game) % 100U) < chance) {
        digiflip_game_injure(mate_game, mate_game->last_update_at, DigiflipInjuryBattle);
        result->mate_injured = true;
    }
    return true;
}
