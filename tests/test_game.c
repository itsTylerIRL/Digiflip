#include "../src/core/digiflip_game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DAY_SECONDS 86400U

static void advance_stage(DigiflipGame* game) {
    game->stage_elapsed = digiflip_stage_duration((DigiflipStage)game->stage);
    assert(digiflip_game_try_evolve(game));
}

static void hatch_egg(DigiflipGame* game) {
    game->stage_elapsed = digiflip_stage_duration(DigiflipStageEgg);
    assert(digiflip_game_try_evolve(game));
}

/* Poke a fresh game straight to Child Agumon; caller sets vitals. */
static void make_agumon(DigiflipGame* game, uint32_t now) {
    digiflip_game_new(game, now);
    hatch_egg(game);
    game->stage = DigiflipStageChild;
    game->species_id = digiflip_species_find("agu");
    game->stage_elapsed = 0;
    game->awake_seconds = 0;
    game->hunger = 4;
    game->strength = 4;
    game->dp_quarters = digiflip_game_dp_max(game);
    game->last_update_at = now;
}

static void test_version_one_lifecycle(void) {
    DigiflipGame game;
    digiflip_game_new(&game, 1000);
    assert(game.stage == DigiflipStageEgg);
    assert(!digiflip_game_feed_meat(&game));

    game.stage_elapsed = digiflip_stage_duration(DigiflipStageEgg);
    assert(digiflip_game_try_evolve(&game));
    assert(strcmp(digiflip_game_species_name(&game), "Botamon") == 0);

    advance_stage(&game);
    assert(strcmp(digiflip_game_species_name(&game), "Koromon") == 0);

    game.care_mistakes = 1;
    advance_stage(&game);
    assert(strcmp(digiflip_game_species_name(&game), "Agumon") == 0);

    game.care_mistakes = 1;
    game.trainings = 16;
    advance_stage(&game);
    assert(strcmp(digiflip_game_species_name(&game), "Greymon") == 0);
}

static void test_care_and_actions(void) {
    DigiflipGame game;
    digiflip_game_new(&game, 0);
    game.stage_elapsed = 60;
    assert(digiflip_game_try_evolve(&game));

    game.hunger = 3;
    game.strength = 3;
    assert(digiflip_game_food_fills(&game, DigiflipFoodMeat));
    assert(digiflip_game_food_fills(&game, DigiflipFoodProtein));
    assert(digiflip_game_feed_meat(&game));
    assert(!digiflip_game_food_fills(&game, DigiflipFoodMeat));
    assert(game.hunger == 4);
    assert(digiflip_game_feed_meat(&game));
    assert(game.overfeeds == 1);
    assert(!digiflip_game_feed_meat(&game));

    game.strength = 1;
    assert(!digiflip_game_train(&game, DIGIFLIP_TRAIN_PRESSES_NEEDED - 1U));
    assert(game.strength == 1);
    const uint8_t weight = game.weight;
    assert(digiflip_game_train(&game, DIGIFLIP_TRAIN_PRESSES_NEEDED));
    assert(game.strength == 3); /* two hearts per success */
    assert(game.weight < weight && game.weight >= weight - 4);
    assert(digiflip_game_train(&game, 40));
    assert(game.strength == 4);
    digiflip_game_train(&game, 0);
    assert(game.effort == 1); /* every session counts, pass or fail */
}

static void test_offline_progress(void) {
    /* 6h offline at Child: hearts empty in 2h, one latched call -> 1 mistake,
       4 waste piles at 2h -> injured once, no deaths yet. */
    DigiflipGame game;
    make_agumon(&game, 160);

    digiflip_game_tick(&game, 160 + 6U * 3600U);
    assert(game.last_update_at == 160 + 6U * 3600U);
    assert(game.alive);
    assert(game.messes == 4);
    assert(game.hunger == 0);
    assert(game.strength == 0);
    assert(game.care_mistakes == 1);
    assert(digiflip_game_is_injured(&game));
    assert(digiflip_game_injuries(&game) == 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
}

static void test_call_state_machine(void) {
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.hunger = 1;

    /* One heart decays after 30 min -> hunger call fires. */
    digiflip_game_tick(&game, 1000 + 1800U);
    assert(game.hunger == 0);
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);

    /* Ignored for 10 minutes -> exactly one care mistake, call ends. */
    digiflip_game_tick(&game, 1000 + 1800U + 600U);
    assert(game.care_mistakes == 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);

    /* No repeat call for the same empty meter. */
    digiflip_game_tick(&game, 1000 + 1800U + 600U + 3600U);
    assert(game.care_mistakes == 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);

    /* Feeding clears the latch; emptying again calls again. */
    assert(digiflip_game_feed_meat(&game));
    digiflip_game_tick(&game, game.last_update_at + 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
    game.hunger = 0;
    digiflip_game_tick(&game, game.last_update_at + 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);

    /* Answering the call resolves it without a mistake. */
    assert(digiflip_game_feed_meat(&game));
    digiflip_game_tick(&game, game.last_update_at + 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
    assert(game.care_mistakes == 1);
}

static void test_one_mistake_both_meters(void) {
    /* Both meters empty at once -> a single care mistake, not two. */
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.hunger = 0;
    game.strength = 0;

    digiflip_game_tick(&game, 1001);
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);
    digiflip_game_tick(&game, 1001 + 601U);
    assert(game.care_mistakes == 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
    digiflip_game_tick(&game, 1001 + 601U + 3600U);
    assert(game.care_mistakes == 1);
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
}

static void test_baby_mistake_immunity(void) {
    /* The hatch call fires during Baby I but an expired call there never
       counts a care mistake (Baby I lasts exactly one call timeout, so the
       expiry is forced while the stage is still Baby I). */
    DigiflipGame game;
    digiflip_game_new(&game, 1000);
    hatch_egg(&game); /* Botamon, hunger = strength = 0 */
    game.last_update_at = 1000;

    digiflip_game_tick(&game, 1001);
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);

    game.call_deadline = game.last_update_at; /* already expired */
    digiflip_game_tick(&game, game.last_update_at + 1);
    assert(game.care_mistakes == 0);
}

static void test_sleep_cycle(void) {
    /* Agumon bedtime is 21:00. Day 10, 20:59 -> not tired yet. */
    const uint32_t evening = 10U * DAY_SECONDS + 20U * 3600U + 59U * 60U;
    DigiflipGame game;
    make_agumon(&game, evening);

    digiflip_game_tick(&game, evening + 120U); /* 21:01 */
    assert(digiflip_game_is_tired(&game));
    assert(digiflip_game_call_reason(&game) == DigiflipCallTired);

    const uint32_t bedtime = evening + 120U;
    const uint32_t wake_at = 11U * DAY_SECONDS + 7U * 3600U;
    assert(digiflip_game_sleep(&game, bedtime));
    assert(digiflip_game_is_asleep(&game));
    assert(!digiflip_game_is_tired(&game));
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
    assert(game.sleep_wake_at == wake_at);
    assert(!game.lights_on);

    const uint32_t elapsed_before = game.stage_elapsed;
    game.dp_quarters = 0;
    digiflip_game_tick(&game, wake_at - 1);
    assert(digiflip_game_is_asleep(&game));
    assert(game.stage_elapsed == elapsed_before); /* frozen while asleep */
    assert(game.hunger == 4); /* no decay while asleep */

    digiflip_game_tick(&game, wake_at);
    assert(!digiflip_game_is_asleep(&game));
    assert(game.lights_on);
    assert(game.dp_quarters == digiflip_game_dp_max(&game)); /* 3h+ sleep restores DP */

    /* Ignoring the tired call for 30 minutes is a care mistake, and the pet
       falls asleep on its own with the lights still on. */
    DigiflipGame late;
    make_agumon(&late, evening);
    digiflip_game_tick(&late, bedtime);
    assert(digiflip_game_call_reason(&late) == DigiflipCallTired);
    digiflip_game_tick(&late, bedtime + 1800U);
    assert(late.care_mistakes == 1);
    assert(digiflip_game_call_reason(&late) == DigiflipCallNone);
    assert(!digiflip_game_is_tired(&late));
    assert(digiflip_game_is_asleep(&late));
    assert(late.lights_on);
    assert(late.sleep_wake_at == wake_at);
    assert(!digiflip_game_sleep(&late, bedtime + 1800U)); /* already asleep */
    /* The lights can be turned off without waking it. */
    assert(digiflip_game_lights_off(&late));
    assert(!late.lights_on && digiflip_game_is_asleep(&late));
    assert(!digiflip_game_lights_off(&late));
    /* It wakes at 7:00 as usual, lights back on. */
    digiflip_game_tick(&late, wake_at);
    assert(!digiflip_game_is_asleep(&late) && late.lights_on);
}

static void test_nap(void) {
    /* A daytime nap (not tired) wakes automatically after 3 hours. */
    const uint32_t noon = 10U * DAY_SECONDS + 12U * 3600U;
    DigiflipGame game;
    make_agumon(&game, noon);
    game.dp_quarters = 0;

    assert(digiflip_game_sleep(&game, noon));
    assert(game.sleep_wake_at == noon + 3U * 3600U);
    digiflip_game_tick(&game, noon + 3U * 3600U);
    assert(!digiflip_game_is_asleep(&game));
    assert(game.dp_quarters == digiflip_game_dp_max(&game));

    /* Waking early (< 3h) does not restore DP. */
    assert(digiflip_game_sleep(&game, noon + 3U * 3600U));
    game.dp_quarters = 0;
    assert(digiflip_game_wake(&game, noon + 3U * 3600U + 3600U));
    assert(game.dp_quarters == 0);
    assert(!digiflip_game_wake(&game, noon + 3U * 3600U + 3601U));
}

static void test_nap_crossing_bedtime(void) {
    /* A nap that would cross bedtime becomes night sleep until 7 AM. */
    const uint32_t evening = 10U * DAY_SECONDS + 19U * 3600U; /* 19:00 */
    DigiflipGame game;
    make_agumon(&game, evening);

    assert(digiflip_game_sleep(&game, evening));
    assert(game.sleep_wake_at == 11U * DAY_SECONDS + 7U * 3600U);
}

static void test_actions_blocked_while_asleep(void) {
    const uint32_t noon = 10U * DAY_SECONDS + 12U * 3600U;
    DigiflipGame game;
    make_agumon(&game, noon);
    game.hunger = 3;
    assert(digiflip_game_sleep(&game, noon));

    assert(!digiflip_game_feed_meat(&game));
    assert(!digiflip_game_feed_protein(&game));
    assert(!digiflip_game_train(&game, DIGIFLIP_TRAIN_PRESSES_NEEDED));
    assert(!digiflip_game_battle(&game, 0, NULL));
    assert(!digiflip_game_sleep(&game, noon + 1));
    assert(digiflip_game_clean(&game) == (game.messes > 0));
}

static void test_sleep_clears_call(void) {
    /* Sleeping with a call active: no mistake accrues during sleep, and the
       call resumes fresh after waking. */
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.hunger = 0;
    digiflip_game_tick(&game, 5001);
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);

    assert(digiflip_game_sleep(&game, 5001));
    assert(digiflip_game_call_reason(&game) == DigiflipCallNone);
    digiflip_game_tick(&game, 5001 + 3600U);
    assert(game.care_mistakes == 0);
    assert(digiflip_game_is_asleep(&game));

    const uint32_t wake_at = game.sleep_wake_at;
    digiflip_game_tick(&game, wake_at);
    assert(!digiflip_game_is_asleep(&game));
    assert(digiflip_game_call_reason(&game) == DigiflipCallHunger);
    digiflip_game_tick(&game, wake_at + 601U);
    assert(game.care_mistakes == 1);
}

static void test_dp_and_battle(void) {
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.dp_quarters = 0;

    DigiflipBattleResult result;
    assert(digiflip_game_battle_block(&game) == DigiflipBattleNoDp);
    assert(!digiflip_game_battle(&game, 0, &result)); /* no DP */
    assert(!result.injured);

    for(unsigned i = 0; i < 4; i++)
        assert(digiflip_game_feed_protein(&game));
    assert(game.dp_quarters == 4);
    assert(digiflip_game_battle(&game, 0, NULL)); /* costs 1 DP */
    assert(game.dp_quarters == 0);
    assert(game.total_battles == 1);

    game.injured = true;
    game.dp_quarters = 4;
    assert(digiflip_game_battle_block(&game) == DigiflipBattleInjured);
    assert(!digiflip_game_battle(&game, 0, NULL)); /* injured: no battles */
    assert(game.dp_quarters == 4);
}

static void test_power_formula(void) {
    DigiflipGame game;
    make_agumon(&game, 1000); /* Agumon base power 18 */
    game.strength = 0;
    assert(digiflip_game_power(&game) == 18);
    game.strength = 4;
    game.weight = 98;
    assert(digiflip_game_power(&game) == 18 + 16);
    game.weight = DIGIFLIP_MAX_WEIGHT; /* 99G forfeits the strength bonus */
    assert(digiflip_game_power(&game) == 18);
}

static void test_attack_ranks(void) {
    assert(digiflip_battle_attack_for_presses(0) == DigiflipAttackWeak);
    assert(digiflip_battle_attack_for_presses(4) == DigiflipAttackStrong);
    assert(digiflip_battle_attack_for_presses(8) == DigiflipAttackDoubleWeak);
    assert(digiflip_battle_attack_for_presses(12) == DigiflipAttackDoubleStrong);
    assert(
        digiflip_battle_attack_for_presses(DIGIFLIP_BATTLE_CHARGE_PRESSES) ==
        DigiflipAttackCritical);
    assert(digiflip_battle_attack_for_presses(200) == DigiflipAttackCritical);
}

static void colosseum_ready(DigiflipGame* game) {
    game->dp_quarters = digiflip_game_dp_max(game);
    game->injured = false;
    game->injuries = 0;
}

static void test_colosseum_ladder(void) {
    /* Wins advance the round, losses retry it, and the ladder wraps. */
    DigiflipGame game;
    make_agumon(&game, 1000);
    DigiflipBattleResult result;
    unsigned wins = 0;
    for(unsigned i = 0; i < 20; i++) {
        colosseum_ready(&game);
        game.battle_round = 2; /* Betamon, power 10 */
        assert(digiflip_game_battle(&game, DIGIFLIP_BATTLE_CHARGE_PRESSES, &result));
        assert(result.round == 2);
        assert(strcmp(result.opponent->name, "Betamon") == 0);
        assert(result.player_species == digiflip_species_find("agu"));
        assert(
            result.exchange_count > 0 && result.exchange_count <= DIGIFLIP_BATTLE_MAX_EXCHANGES);
        assert(result.exchanges[0].by_player);
        if(result.victory) {
            wins++;
            assert(game.battle_round == 3);
        }
    }
    assert(wins >= 15); /* Agumon 34 vs Betamon 10 should dominate */

    unsigned losses = 0;
    for(unsigned i = 0; i < 20; i++) {
        colosseum_ready(&game);
        game.battle_round = DIGIFLIP_COLOSSEUM_ROUNDS - 1U; /* Lucemon, power 210 */
        assert(digiflip_game_battle(&game, 0, &result));
        if(!result.victory) {
            losses++;
            assert(game.battle_round == DIGIFLIP_COLOSSEUM_ROUNDS - 1U);
        } else {
            assert(game.battle_round == 0); /* cleared the ladder: wrap */
        }
    }
    assert(losses >= 15);
    assert(game.total_battles == 40);
}

static void test_attribute_hit_rate(void) {
    /* Agumon (Vaccine 18+16) vs Kunemon (Virus 10): advantage adds 32. */
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.battle_round = 0;
    DigiflipBattleResult result;
    assert(digiflip_game_battle(&game, 0, &result));
    assert(result.hit_rate == (34U + 32U) * 100U / (34U + 32U + 10U));
}

static void test_waste_injury_and_heal(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.messes = 3;

    digiflip_game_tick(&game, 5000 + 1801U);
    assert(game.messes == 4);
    assert(digiflip_game_is_injured(&game));
    assert(digiflip_game_injuries(&game) == 1);

    /* No repeat injury while already injured. */
    digiflip_game_tick(&game, 5000 + 1801U + 7200U);
    assert(digiflip_game_injuries(&game) == 1);

    /* Medicine: 50% per dose, eventually cures. */
    unsigned tries = 0;
    while(digiflip_game_is_injured(&game) && tries < 64) {
        digiflip_game_heal(&game);
        tries++;
    }
    assert(!digiflip_game_is_injured(&game));
    assert(!digiflip_game_heal(&game)); /* not injured: no dose */
}

static void test_starve_death(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.hunger = 0;
    game.next_poop_at = 5000 + 13U * 3600U; /* isolate starvation from waste */

    digiflip_game_tick(&game, 5000 + 12U * 3600U + 1U);
    assert(!game.alive);
    assert(digiflip_game_death_cause(&game) == DigiflipDeathStarved);
}

static void test_injured_death(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.injured = true;
    game.injured_at = 5000;
    game.injuries = 1;

    digiflip_game_tick(&game, 5000 + 6U * 3600U + 1U);
    assert(!game.alive);
    assert(digiflip_game_death_cause(&game) == DigiflipDeathInjuredTooLong);
}

static void test_neglect_death(void) {
    /* 5th care mistake after 48h awake in this stage -> death. */
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.care_mistakes = 4;
    game.awake_seconds = 48U * 3600U;
    game.hunger = 0;

    digiflip_game_tick(&game, 5000 + 601U);
    assert(!game.alive);
    assert(digiflip_game_death_cause(&game) == DigiflipDeathNeglect);
}

static void test_twenty_mistakes_death(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.care_mistakes = 19;
    game.hunger = 0;

    digiflip_game_tick(&game, 5000 + 601U);
    assert(!game.alive);
    assert(digiflip_game_death_cause(&game) == DigiflipDeathCareMistakes);
}

static void test_twenty_injuries_death(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.injuries = 19;
    game.messes = 3;

    digiflip_game_tick(&game, 5000 + 1801U);
    assert(!game.alive);
    assert(digiflip_game_death_cause(&game) == DigiflipDeathInjuries);
}

static void test_evolution_paused_in_sleep(void) {
    /* 1h of Child time left; sleep through the deadline -> no evolution
       until after waking. */
    const uint32_t morning = 10U * DAY_SECONDS + 8U * 3600U;
    DigiflipGame game;
    make_agumon(&game, morning);
    game.care_mistakes = 1;
    game.stage_elapsed = 24U * 3600U - 3600U;

    assert(digiflip_game_sleep(&game, morning));
    const uint32_t wake_at = morning + 3U * 3600U;
    digiflip_game_tick(&game, morning + 7200U);
    assert(game.stage == DigiflipStageChild);
    assert(game.stage_elapsed == 24U * 3600U - 3600U);

    digiflip_game_tick(&game, wake_at + 3601U);
    assert(!digiflip_game_is_asleep(&game));
    assert(game.stage == DigiflipStageAdult);
}

static void test_chronological_offline_evolution(void) {
    /* BabyII -> Child due in 1h; 6h offline evolves once with overflow. */
    DigiflipGame game;
    digiflip_game_new(&game, 5000);
    hatch_egg(&game); /* Botamon */
    game.stage_elapsed = digiflip_stage_duration(DigiflipStageBabyI);
    assert(digiflip_game_try_evolve(&game)); /* Koromon */
    assert(game.stage == DigiflipStageBabyII);
    game.stage_elapsed = 6U * 3600U - 3600U;
    game.hunger = 4;
    game.strength = 4;
    game.last_update_at = 5000;

    digiflip_game_tick(&game, 5000 + 6U * 3600U);
    assert(game.stage == DigiflipStageChild);
    assert(game.stage_elapsed == 5U * 3600U);
    assert(game.alive);
}

static void test_last_fifteen_battles(void) {
    DigiflipGame game;
    digiflip_game_new(&game, 0);
    game.stage = DigiflipStageAdult;
    game.species_id = digiflip_species_find("grey");
    for(unsigned i = 0; i < 20; i++) {
        digiflip_game_record_battle(&game, i >= 8, DIGIFLIP_SPECIES_NONE);
    }
    assert(game.battles == 20);
    assert(game.battle_history_count == 15);
    assert(digiflip_game_recent_victories(&game) == 12);
}

static unsigned stage_v_successes(unsigned wins, bool traited) {
    unsigned evolved = 0;
    for(uint32_t seed = 1; seed <= 1000; seed++) {
        DigiflipGame game;
        digiflip_game_new_with_egg(&game, seed * 7919U, DigiflipEggVersion1, traited);
        game.stage = DigiflipStageAdult;
        game.species_id = digiflip_species_find("grey");
        for(unsigned i = 0; i < 15; i++) {
            digiflip_game_record_battle(&game, i < wins, DIGIFLIP_SPECIES_NONE);
        }
        game.stage_elapsed = digiflip_stage_duration(DigiflipStageAdult);
        if(digiflip_game_try_evolve(&game) && game.stage == DigiflipStagePerfect) evolved++;
        assert(game.evolution_attempted || game.stage == DigiflipStagePerfect);
    }
    return evolved;
}

static void test_stage_v_chance(void) {
    assert(digiflip_stage_v_chance(5, false) == 0);
    assert(digiflip_stage_v_chance(6, false) == 10);
    assert(digiflip_stage_v_chance(11, false) == 60);
    assert(digiflip_stage_v_chance(11, true) == 70);
    assert(digiflip_stage_v_chance(12, false) == 100);

    assert(stage_v_successes(12, false) == 1000);
    assert(stage_v_successes(5, false) == 0);
    assert(stage_v_successes(5, true) == 0);
    const unsigned plain = stage_v_successes(8, false); /* 30% */
    const unsigned traited = stage_v_successes(8, true); /* 40% */
    assert(plain > 240 && plain < 360);
    assert(traited > 330 && traited < 470);
}

static void test_traited_egg(void) {
    DigiflipGame game;
    make_agumon(&game, 5000);
    game.evolution_attempted = true;
    game.stage_elapsed = DIGIFLIP_TRAIT_AWAKE_SECONDS;
    game.injured = true;
    game.injured_at = 5000;
    game.injuries = 1;
    digiflip_game_tick(&game, 5000 + 6U * 3600U + 1U);
    assert(!game.alive && game.trait_earned);

    make_agumon(&game, 5000);
    game.evolution_attempted = true;
    game.injured = true;
    game.injured_at = 5000;
    game.injuries = 1;
    digiflip_game_tick(&game, 5000 + 6U * 3600U + 1U);
    assert(!game.alive && !game.trait_earned);

    digiflip_game_new_with_egg(&game, 5000, DigiflipEggVersion2, true);
    assert(game.traited && !game.trait_earned && game.egg_id == DigiflipEggVersion2);
}

static void test_evo_pending(void) {
    DigiflipGame game;
    digiflip_game_new(&game, 1000);
    assert(!digiflip_game_evo_pending(&game));

    hatch_egg(&game);
    assert(digiflip_game_evo_pending(&game));
    assert(digiflip_game_evo_from_species(&game) == DIGIFLIP_SPECIES_NONE);
    digiflip_game_clear_evo_pending(&game);
    assert(!digiflip_game_evo_pending(&game));

    const DigiflipSpeciesId botamon = game.species_id;
    game.stage_elapsed = digiflip_stage_duration((DigiflipStage)game.stage);
    assert(digiflip_game_try_evolve(&game));
    assert(digiflip_game_evo_pending(&game));
    assert(digiflip_game_evo_from_species(&game) == botamon);
    digiflip_game_clear_evo_pending(&game);
    assert(!digiflip_game_evo_pending(&game));
}

/* Event capture for the log tests. */
static DigiflipEvent captured[256];
static unsigned captured_count;

static const DigiflipGame* captured_game;

static void capture(void* context, const DigiflipGame* game, const DigiflipEvent* event) {
    (void)context;
    captured_game = game;
    if(captured_count < sizeof(captured) / sizeof(captured[0]))
        captured[captured_count++] = *event;
}

static const DigiflipEvent* find_event(DigiflipEventType type) {
    for(unsigned i = 0; i < captured_count; i++) {
        if(captured[i].type == type) return &captured[i];
    }
    return NULL;
}

static void test_need_rates(void) {
    DigiflipGame game;
    digiflip_game_new(&game, 0);
    hatch_egg(&game); /* Baby I: the neediest */
    assert(digiflip_game_heart_interval(&game, 60) == 3U * 60U);
    assert(digiflip_game_poop_interval(&game, 60) == 5U * 60U);
    make_agumon(&game, 1000); /* born at 1000 */
    assert(digiflip_game_heart_interval(&game, 1000) == DIGIFLIP_HEART_DECAY_SECONDS);
    assert(digiflip_game_poop_interval(&game, 1000) == DIGIFLIP_POOP_INTERVAL_SECONDS);
    game.stage = DigiflipStagePerfect;
    assert(digiflip_game_heart_interval(&game, 1000) == 60U * 60U);
    /* Old age (6+ days) speeds things up again. */
    const uint32_t old = 1000U + DIGIFLIP_OLD_AGE_DAYS * DAY_SECONDS;
    assert(digiflip_game_heart_interval(&game, old - 1U) == 60U * 60U);
    assert(digiflip_game_heart_interval(&game, old) == 40U * 60U);
    assert(digiflip_game_poop_interval(&game, old) == 40U * 60U);

    /* The engine actually uses the stage rate: a Baby I empties fast. */
    digiflip_game_new(&game, 5000);
    hatch_egg(&game);
    game.hunger = 1;
    game.last_update_at = 5000;
    digiflip_game_tick(&game, 5000U + 3U * 60U);
    assert(game.hunger == 0);
}

static void test_tag_training(void) {
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.strength = 1;
    assert(!digiflip_game_tag_train(&game, DIGIFLIP_TAG_TRAIN_NEEDED - 1U));
    assert(game.strength == 1 && game.trainings == 1);
    assert(digiflip_game_tag_train(&game, DIGIFLIP_TAG_TRAIN_NEEDED));
    assert(game.strength == 3 && game.trainings == 2);
    /* The guard is random but both sides come up. */
    unsigned high = 0;
    for(unsigned i = 0; i < 64; i++)
        high += digiflip_game_tag_guard_high(&game) ? 1U : 0U;
    assert(high > 0 && high < 64);
}

static void ready_for_battle(DigiflipGame* game) {
    game->dp_quarters = digiflip_game_dp_max(game);
    game->injured = false;
    game->injuries = 0;
}

static void test_tag_battles(void) {
    DigiflipGame game;
    make_agumon(&game, 1000);
    const DigiflipSpeciesId partner = digiflip_species_find("gabu");
    DigiflipBattleResult result;
    unsigned wins = 0;
    for(unsigned i = 0; i < 20; i++) {
        ready_for_battle(&game);
        game.tag_round = 0; /* Palmon & Kunemon */
        assert(digiflip_game_tag_battle(
            &game, partner, DIGIFLIP_BATTLE_CHARGE_PRESSES, DigiflipSyncPerfect, &result));
        assert(result.tag && result.partner_species == partner);
        assert(strcmp(result.opponent->name, "Palmon") == 0);
        assert(strcmp(result.opponent2->name, "Kunemon") == 0);
        assert(result.exchanges[0].attacker == DigiflipFighterPet);
        assert(result.exchange_count < 2 || result.exchanges[1].attacker == DigiflipFighterLeft);
        if(result.victory) {
            wins++;
            assert(game.tag_round == 1);
        }
    }
    assert(wins >= 15);
    assert(game.battle_round == 0); /* the single ladder is separate */
    assert(game.tag_partner == partner && game.tag_battles == 20);

    /* A miss on the cursor costs an attack rank, a perfect stop adds one. */
    ready_for_battle(&game);
    assert(digiflip_game_tag_battle(&game, partner, 0, DigiflipSyncMiss, &result));
    assert(result.attack == DigiflipAttackWeak);
    ready_for_battle(&game);
    assert(digiflip_game_tag_battle(&game, partner, 0, DigiflipSyncPerfect, &result));
    assert(result.attack == DigiflipAttackStrong);

    /* No partner, no tag battle; and the usual blocks apply. */
    ready_for_battle(&game);
    assert(!digiflip_game_tag_battle(&game, DIGIFLIP_SPECIES_NONE, 0, DigiflipSyncNear, NULL));
    game.dp_quarters = 0;
    assert(!digiflip_game_tag_battle(&game, partner, 0, DigiflipSyncNear, NULL));
}

static void test_jogress(void) {
    /* Blitz Greymon + Cres Garurumon, 5 tag battles -> Omegamon Alter S. */
    DigiflipGame game;
    make_agumon(&game, 1000);
    game.species_id = digiflip_species_find("blitzgrey");
    game.stage = DigiflipStageUltimate;
    game.dp_quarters = digiflip_game_dp_max(&game);
    const DigiflipSpeciesId partner = digiflip_species_find("cresgaruru");
    const DigiflipSpeciesId wrong = digiflip_species_find("agu");

    /* Tag battles with the wrong partner don't count toward it. */
    for(unsigned i = 0; i < 5; i++) {
        ready_for_battle(&game);
        assert(digiflip_game_tag_battle(&game, wrong, 0, DigiflipSyncNear, NULL));
    }
    assert(game.species_id == digiflip_species_find("blitzgrey"));

    DigiflipBattleResult result;
    for(unsigned i = 0; i < 5; i++) {
        ready_for_battle(&game);
        assert(game.species_id == digiflip_species_find("blitzgrey"));
        assert(digiflip_game_tag_battle(&game, partner, 0, DigiflipSyncNear, &result));
    }
    assert(result.evolved);
    assert(game.species_id == digiflip_species_find("omega_a"));
    assert(game.stage == DigiflipStageSuperUltimate);
    assert(result.player_species == digiflip_species_find("blitzgrey"));
}

static void test_team_tag_battle(void) {
    /* Two raised pets: both pay DP and both count each other as partner. */
    DigiflipGame lead;
    DigiflipGame mate;
    make_agumon(&lead, 1000);
    make_agumon(&mate, 1000);
    mate.species_id = digiflip_species_find("gabu");
    const uint16_t dp = digiflip_game_dp_max(&lead);
    DigiflipBattleResult result;
    assert(digiflip_game_tag_battle_team(
        &lead, &mate, DIGIFLIP_SPECIES_NONE, 0, DigiflipSyncNear, &result));
    assert(result.partner_species == digiflip_species_find("gabu"));
    assert(lead.dp_quarters == dp - DIGIFLIP_BATTLE_DP_QUARTERS);
    assert(mate.dp_quarters == dp - DIGIFLIP_BATTLE_DP_QUARTERS);
    assert(lead.tag_partner == digiflip_species_find("gabu") && lead.tag_battles == 1);
    assert(mate.tag_partner == digiflip_species_find("agu") && mate.tag_battles == 1);
    assert(mate.tag_round == 0); /* the ladder belongs to the lead */

    /* Either pet being unable to battle blocks it. */
    ready_for_battle(&lead);
    ready_for_battle(&mate);
    mate.injured = true;
    assert(!digiflip_game_tag_battle_team(
        &lead, &mate, DIGIFLIP_SPECIES_NONE, 0, DigiflipSyncNear, NULL));

    /* Jogress works from either side: two raised Ultimates. */
    DigiflipGame blitz;
    DigiflipGame cres;
    make_agumon(&blitz, 1000);
    make_agumon(&cres, 1000);
    blitz.species_id = digiflip_species_find("blitzgrey");
    blitz.stage = DigiflipStageUltimate;
    cres.species_id = digiflip_species_find("cresgaruru");
    cres.stage = DigiflipStageUltimate;
    for(unsigned i = 0; i < 5; i++) {
        ready_for_battle(&blitz);
        ready_for_battle(&cres);
        assert(digiflip_game_tag_battle_team(
            &blitz, &cres, DIGIFLIP_SPECIES_NONE, 0, DigiflipSyncNear, &result));
    }
    assert(result.evolved && result.mate_evolved);
    assert(blitz.species_id == digiflip_species_find("omega_a"));
    assert(cres.species_id == digiflip_species_find("omega_a"));
}

static void test_events(void) {
    captured_count = 0;
    digiflip_game_set_event_sink(capture, NULL);

    DigiflipGame game;
    digiflip_game_new(&game, 1000);
    assert(find_event(DigiflipEventNewEgg));
    hatch_egg(&game);
    const DigiflipEvent* hatched = find_event(DigiflipEventHatched);
    assert(hatched && hatched->b == digiflip_species_find("bota"));

    /* Offline events carry the time they happened, not the catch-up time. */
    const uint32_t start = 50000;
    make_agumon(&game, start);
    game.hunger = 1;
    game.strength = 1;
    captured_count = 0;
    digiflip_game_tick(&game, start + 3U * 3600U);
    const DigiflipEvent* empty = find_event(DigiflipEventHungerEmpty);
    assert(empty && empty->time == start + DIGIFLIP_HEART_DECAY_SECONDS);
    const DigiflipEvent* call = find_event(DigiflipEventCall);
    assert(call && call->a == DigiflipCallHunger && call->time == empty->time);
    const DigiflipEvent* mistake = find_event(DigiflipEventCareMistake);
    assert(mistake && mistake->time == call->time + DIGIFLIP_CALL_TIMEOUT_SECONDS);
    assert(find_event(DigiflipEventPooped));
    /* Chronological order. */
    for(unsigned i = 1; i < captured_count; i++)
        assert(captured[i].time >= captured[i - 1].time);

    /* Actions stamp the current time. */
    captured_count = 0;
    assert(digiflip_game_feed_meat(&game));
    const DigiflipEvent* fed = find_event(DigiflipEventFed);
    assert(fed && fed->a == DigiflipFoodMeat && fed->time == game.last_update_at);

    /* A call that's answered in time is logged as answered. */
    make_agumon(&game, 90000);
    game.hunger = 0;
    captured_count = 0;
    digiflip_game_tick(&game, 90001);
    assert(find_event(DigiflipEventCall));
    assert(digiflip_game_feed_meat(&game));
    digiflip_game_tick(&game, 90002);
    const DigiflipEvent* answered = find_event(DigiflipEventCallAnswered);
    assert(answered && answered->a == DigiflipCallHunger);
    assert(!find_event(DigiflipEventCareMistake));

    digiflip_game_set_event_sink(NULL, NULL);
}

static void test_cheats(void) {
    const DigiflipRules cheats = {.freeze_hearts = true, .no_waste = true};
    const DigiflipRules normal = {0};
    DigiflipGame game;
    make_agumon(&game, 50000);
    game.hunger = 2;
    game.strength = 3;
    digiflip_game_set_rules(&cheats);
    digiflip_game_tick(&game, 50000 + 5U * 3600U);
    assert(game.hunger == 2 && game.strength == 3); /* frozen */
    assert(game.messes == 0); /* no poop */
    digiflip_game_set_rules(&normal);
    digiflip_game_tick(&game, 50000 + 7U * 3600U);
    assert(game.hunger < 2 && game.messes > 0);

    digiflip_game_fill_hearts(&game);
    assert(game.hunger == DIGIFLIP_MAX_HEARTS && game.strength == DIGIFLIP_MAX_HEARTS);

    /* Forced evolution: egg hatches, Child becomes an Adult, final forms can't. */
    digiflip_game_new(&game, 1000);
    assert(digiflip_game_force_evolve(&game));
    assert(game.stage == DigiflipStageBabyI);
    make_agumon(&game, 1000);
    assert(digiflip_game_force_evolve(&game));
    assert(game.stage == DigiflipStageAdult);
    assert(digiflip_game_evo_pending(&game));
    assert(digiflip_game_evo_from_species(&game) == digiflip_species_find("agu"));
    game.species_id = digiflip_species_find("omega");
    game.stage = DigiflipStageSuperUltimate;
    assert(!digiflip_game_force_evolve(&game));
}

int main(void) {
    test_version_one_lifecycle();
    test_stage_v_chance();
    test_traited_egg();
    test_care_and_actions();
    test_offline_progress();
    test_call_state_machine();
    test_one_mistake_both_meters();
    test_baby_mistake_immunity();
    test_sleep_cycle();
    test_nap();
    test_nap_crossing_bedtime();
    test_sleep_clears_call();
    test_actions_blocked_while_asleep();
    test_dp_and_battle();
    test_power_formula();
    test_attack_ranks();
    test_colosseum_ladder();
    test_attribute_hit_rate();
    test_waste_injury_and_heal();
    test_starve_death();
    test_injured_death();
    test_neglect_death();
    test_twenty_mistakes_death();
    test_twenty_injuries_death();
    test_evolution_paused_in_sleep();
    test_chronological_offline_evolution();
    test_last_fifteen_battles();
    test_evo_pending();
    test_need_rates();
    test_tag_training();
    test_tag_battles();
    test_jogress();
    test_team_tag_battle();
    test_events();
    test_cheats();
    puts("digiflip game tests passed");
    return 0;
}
