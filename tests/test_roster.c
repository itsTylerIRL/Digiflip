#include "../src/core/digiflip_roster.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static DigiflipSpeciesId species(const char* key) {
    const DigiflipSpeciesId id = digiflip_species_find(key);
    assert(id != DIGIFLIP_SPECIES_NONE);
    return id;
}

static void test_roster_shape(void) {
    assert(digiflip_species_count == 134);
    assert(digiflip_evolution_rule_count == 180);
    assert(strcmp(digiflip_species_get(species("bota"))->name, "Botamon") == 0);
    assert(strcmp(digiflip_species_get(species("omega"))->name, "Omegamon") == 0);
}

static void test_version_one_branches(void) {
    DigiflipEvolutionContext context = {
        .care_mistakes = 1,
        .trainings = 16,
        .egg = DigiflipEggVersion1,
    };
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(species("agu"), &context);
    assert(rule && rule->to == species("grey"));

    context.care_mistakes = 4;
    context.trainings = 7;
    context.overfeeds = 3;
    rule = digiflip_evolution_match(species("agu"), &context);
    assert(rule && rule->to == species("tyrano"));

    context.trainings = 2;
    rule = digiflip_evolution_match(species("agu"), &context);
    assert(rule && rule->to == species("nume"));
}

static void test_egg_specific_and_battle_routes(void) {
    DigiflipEvolutionContext context = {.egg = DigiflipEggTaichi};
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(species("koro"), &context);
    assert(rule && rule->to == species("agu_a"));

    context =
        (DigiflipEvolutionContext){.egg = DigiflipEggVersion1, .battles = 20, .victories = 12};
    rule = digiflip_evolution_match(species("grey"), &context);
    assert(rule && rule->to == species("metalgrey_vi"));

    context.victories = 11;
    assert(digiflip_evolution_match(species("grey"), &context) == NULL);
}

static void test_tag_route(void) {
    DigiflipEvolutionContext context = {
        .egg = DigiflipEggVersion1,
        .tag_partner = species("cresgaruru"),
        .tag_battles = 5,
    };
    const DigiflipEvolutionRule* rule = digiflip_evolution_match(species("blitzgrey"), &context);
    assert(rule && rule->to == species("omega_a"));
}

static void test_colosseum(void) {
    const DigiflipColosseumOpponent* first = digiflip_colosseum_opponent(0);
    assert(first && strcmp(first->name, "Kunemon") == 0);
    assert(first->species == digiflip_species_find("kune"));
    assert(first->attribute == DigiflipAttributeVirus && first->power == 10);
    assert(digiflip_colosseum_opponent(DIGIFLIP_COLOSSEUM_ROUNDS) == NULL);
    for(uint8_t round = 0; round < DIGIFLIP_COLOSSEUM_ROUNDS; round++) {
        const DigiflipColosseumOpponent* opponent = digiflip_colosseum_opponent(round);
        assert(opponent->power > 0);
        /* Every opponent has either a roster sprite or a boss sprite. */
        assert(
            (opponent->species == DIGIFLIP_SPECIES_NONE) !=
            (opponent->boss == DIGIFLIP_COLOSSEUM_NO_BOSS));
        if(opponent->species != DIGIFLIP_SPECIES_NONE) {
            const DigiflipSpecies* species = digiflip_species_get(opponent->species);
            assert(strcmp(species->key, opponent->key) == 0);
            assert(species->power == opponent->power);
        }
    }
    assert(digiflip_attribute_advantage(DigiflipAttributeVaccine, DigiflipAttributeVirus));
    assert(digiflip_attribute_advantage(DigiflipAttributeVirus, DigiflipAttributeData));
    assert(digiflip_attribute_advantage(DigiflipAttributeData, DigiflipAttributeVaccine));
    assert(!digiflip_attribute_advantage(DigiflipAttributeVirus, DigiflipAttributeVaccine));
    assert(!digiflip_attribute_advantage(DigiflipAttributeFree, DigiflipAttributeData));
    assert(!digiflip_attribute_advantage(DigiflipAttributeData, DigiflipAttributeFree));
}

static void test_tag_colosseum(void) {
    const DigiflipTagRound* first = digiflip_colosseum_tag_round(0);
    assert(first && strcmp(first->left.name, "Palmon") == 0);
    assert(strcmp(first->right.name, "Kunemon") == 0);
    const DigiflipTagRound* last = digiflip_colosseum_tag_round(DIGIFLIP_COLOSSEUM_ROUNDS - 1U);
    assert(strcmp(last->left.name, "Omegamon") == 0 && last->left.power == 238);
    assert(strcmp(last->right.name, "Alphamon") == 0);
    assert(digiflip_colosseum_tag_round(DIGIFLIP_COLOSSEUM_ROUNDS) == NULL);
    for(uint8_t round = 0; round < DIGIFLIP_COLOSSEUM_ROUNDS; round++) {
        const DigiflipTagRound* r = digiflip_colosseum_tag_round(round);
        const DigiflipColosseumOpponent* pair[2] = {&r->left, &r->right};
        for(int i = 0; i < 2; i++) {
            assert(pair[i]->power > 0);
            assert(
                (pair[i]->species == DIGIFLIP_SPECIES_NONE) !=
                (pair[i]->boss == DIGIFLIP_COLOSSEUM_NO_BOSS));
            if(pair[i]->boss != DIGIFLIP_COLOSSEUM_NO_BOSS) {
                /* Every boss resolves (for its sprite file). */
                assert(digiflip_colosseum_boss(pair[i]->boss) != NULL);
            }
        }
    }
    assert(digiflip_colosseum_boss(0xFEU) == NULL); /* not a boss index */
}

int main(void) {
    test_roster_shape();
    test_version_one_branches();
    test_egg_specific_and_battle_routes();
    test_tag_route();
    test_colosseum();
    test_tag_colosseum();
    puts("digiflip roster tests passed");
    return 0;
}
