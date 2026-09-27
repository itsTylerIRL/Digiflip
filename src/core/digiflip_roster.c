#include "digiflip_roster.h"

#include <string.h>

#define COUNT_OF(items) (sizeof(items) / sizeof((items)[0]))
#include "digiflip_roster_data.inc"
#include "digiflip_colosseum_data.inc"
#undef COUNT_OF

const DigiflipSpecies* digiflip_species_get(DigiflipSpeciesId id) {
    return id < digiflip_species_count ? &digiflip_species[id] : NULL;
}

DigiflipSpeciesId digiflip_species_find(const char* key) {
    for(size_t i = 0; i < digiflip_species_count; i++) {
        if(strcmp(digiflip_species[i].key, key) == 0) return (DigiflipSpeciesId)i;
    }
    return DIGIFLIP_SPECIES_NONE;
}

static bool digiflip_in_range(uint16_t value, uint16_t minimum, uint16_t maximum) {
    return value >= minimum && value <= maximum;
}

static bool digiflip_rule_matches(
    const DigiflipEvolutionRule* rule,
    const DigiflipEvolutionContext* context) {
    if(rule->egg_mask && !(rule->egg_mask & (1U << context->egg))) return false;
    if(!digiflip_in_range(context->care_mistakes, rule->care_min, rule->care_max)) return false;
    if(!digiflip_in_range(context->trainings, rule->training_min, rule->training_max))
        return false;
    if(!digiflip_in_range(context->overfeeds, rule->overfeed_min, rule->overfeed_max))
        return false;
    if(!digiflip_in_range(context->battles, rule->battles_min, rule->battles_max)) return false;
    if(!digiflip_in_range(context->victories, rule->victories_min, rule->victories_max))
        return false;
    if(context->elapsed_seconds < rule->wait_seconds) return false;
    if(rule->tag_partner != DIGIFLIP_SPECIES_NONE &&
       (context->tag_partner != rule->tag_partner || context->tag_battles < rule->tag_battles_min))
        return false;
    return true;
}

const DigiflipEvolutionRule*
    digiflip_evolution_match(DigiflipSpeciesId from, const DigiflipEvolutionContext* context) {
    for(size_t i = 0; i < digiflip_evolution_rule_count; i++) {
        const DigiflipEvolutionRule* rule = &digiflip_evolution_rules[i];
        if(rule->from == from && digiflip_rule_matches(rule, context)) return rule;
    }
    return NULL;
}

uint32_t digiflip_stage_duration(DigiflipStage stage) {
    switch(stage) {
    case DigiflipStageEgg:
        return 60U;
    case DigiflipStageBabyI:
        return 10U * 60U;
    case DigiflipStageBabyII:
        return 6U * 60U * 60U;
    case DigiflipStageChild:
        return 24U * 60U * 60U;
    case DigiflipStageAdult:
        return 36U * 60U * 60U;
    case DigiflipStagePerfect:
        return 48U * 60U * 60U;
    case DigiflipStageUltimate:
    case DigiflipStageSuperUltimate:
        return 0;
    }
    return 0;
}

const char* digiflip_stage_name(DigiflipStage stage) {
    switch(stage) {
    case DigiflipStageEgg:
        return "Egg";
    case DigiflipStageBabyI:
        return "Baby I";
    case DigiflipStageBabyII:
        return "Baby II";
    case DigiflipStageChild:
        return "Child";
    case DigiflipStageAdult:
        return "Adult";
    case DigiflipStagePerfect:
        return "Perfect";
    case DigiflipStageUltimate:
        return "Ultimate";
    case DigiflipStageSuperUltimate:
        return "Super Ultimate";
    }
    return "Unknown";
}

const char* digiflip_attribute_name(DigiflipAttribute attribute) {
    switch(attribute) {
    case DigiflipAttributeVaccine:
        return "Vaccine";
    case DigiflipAttributeData:
        return "Data";
    case DigiflipAttributeVirus:
        return "Virus";
    case DigiflipAttributeFree:
    default:
        return "Free";
    }
}

bool digiflip_attribute_advantage(DigiflipAttribute attacker, DigiflipAttribute defender) {
    return (attacker == DigiflipAttributeVaccine && defender == DigiflipAttributeVirus) ||
           (attacker == DigiflipAttributeVirus && defender == DigiflipAttributeData) ||
           (attacker == DigiflipAttributeData && defender == DigiflipAttributeVaccine);
}

const DigiflipColosseumOpponent* digiflip_colosseum_opponent(uint8_t round) {
    return round < DIGIFLIP_COLOSSEUM_ROUNDS ? &digiflip_colosseum[round] : NULL;
}

const DigiflipTagRound* digiflip_colosseum_tag_round(uint8_t round) {
    return round < DIGIFLIP_COLOSSEUM_ROUNDS ? &digiflip_colosseum_tag[round] : NULL;
}

const DigiflipColosseumOpponent* digiflip_colosseum_boss(uint8_t boss) {
    for(uint8_t round = 0; round < DIGIFLIP_COLOSSEUM_ROUNDS; round++) {
        if(digiflip_colosseum[round].boss == boss) return &digiflip_colosseum[round];
    }
    for(uint8_t round = 0; round < DIGIFLIP_COLOSSEUM_ROUNDS; round++) {
        if(digiflip_colosseum_tag[round].left.boss == boss)
            return &digiflip_colosseum_tag[round].left;
        if(digiflip_colosseum_tag[round].right.boss == boss)
            return &digiflip_colosseum_tag[round].right;
    }
    return NULL;
}
