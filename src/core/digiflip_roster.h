#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DIGIFLIP_SPECIES_NONE   UINT16_MAX
#define DIGIFLIP_RULE_UNBOUNDED UINT16_MAX

typedef uint16_t DigiflipSpeciesId;

typedef enum {
    DigiflipStageEgg,
    DigiflipStageBabyI,
    DigiflipStageBabyII,
    DigiflipStageChild,
    DigiflipStageAdult,
    DigiflipStagePerfect,
    DigiflipStageUltimate,
    DigiflipStageSuperUltimate,
} DigiflipStage;

typedef enum {
    DigiflipAttributeFree,
    DigiflipAttributeVaccine,
    DigiflipAttributeData,
    DigiflipAttributeVirus,
} DigiflipAttribute;

typedef enum {
    DigiflipEggVersion1,
    DigiflipEggVersion2,
    DigiflipEggVersion3,
    DigiflipEggVersion4,
    DigiflipEggVersion5,
    DigiflipEggZuba,
    DigiflipEggHack,
    DigiflipEggSlayerdra,
    DigiflipEggBreakdra,
    DigiflipEggCorona,
    DigiflipEggLuna,
    DigiflipEggTaichi,
    DigiflipEggYamato,
    DigiflipEggDoru,
    DigiflipEggMeicoo,
    DigiflipEggCount,
} DigiflipEgg;

typedef struct {
    const char* key;
    const char* name;
    DigiflipStage stage;
    DigiflipAttribute attribute;
    uint16_t power;
    /* Bedtime in minutes after midnight (pet-local clock); DIGIFLIP_NO_SLEEP
       when the species has no scheduled sleep (babies). Wake is 7:00 AM. */
    uint16_t sleep_minutes;
} DigiflipSpecies;

#define DIGIFLIP_NO_SLEEP     0xFFFFU
#define DIGIFLIP_WAKE_MINUTES (7U * 60U)

typedef struct {
    const char* name;
    DigiflipSpeciesId hatch_species;
} DigiflipEggInfo;

typedef struct {
    DigiflipSpeciesId from;
    DigiflipSpeciesId to;
    uint16_t care_min;
    uint16_t care_max;
    uint16_t training_min;
    uint16_t training_max;
    uint16_t overfeed_min;
    uint16_t overfeed_max;
    uint16_t battles_min;
    uint16_t battles_max;
    uint16_t victories_min;
    uint16_t victories_max;
    uint16_t egg_mask;
    DigiflipSpeciesId tag_partner;
    uint16_t tag_battles_min;
    uint32_t wait_seconds;
    const char* requirement;
} DigiflipEvolutionRule;

typedef struct {
    uint16_t care_mistakes;
    uint16_t trainings;
    uint16_t overfeeds;
    uint16_t battles;
    uint16_t victories;
    DigiflipEgg egg;
    DigiflipSpeciesId tag_partner;
    uint16_t tag_battles;
    uint32_t elapsed_seconds;
} DigiflipEvolutionContext;

/* Single-battle Colosseum (100 sequential rounds). Colosseum-only bosses have
   no roster species; they carry a boss index for their sprite instead. */
#define DIGIFLIP_COLOSSEUM_ROUNDS  100U
#define DIGIFLIP_COLOSSEUM_NO_BOSS 0xFFU

typedef struct {
    const char* key;
    const char* name;
    DigiflipSpeciesId species;
    uint8_t boss;
    DigiflipAttribute attribute;
    uint16_t power;
} DigiflipColosseumOpponent;

/* A tag Colosseum round: two opponents against your pet and Copymon. */
typedef struct {
    DigiflipColosseumOpponent left;
    DigiflipColosseumOpponent right;
} DigiflipTagRound;

extern const DigiflipSpecies digiflip_species[];
extern const size_t digiflip_species_count;
extern const DigiflipEggInfo digiflip_eggs[DigiflipEggCount];
extern const DigiflipEvolutionRule digiflip_evolution_rules[];
extern const size_t digiflip_evolution_rule_count;

const DigiflipSpecies* digiflip_species_get(DigiflipSpeciesId id);
DigiflipSpeciesId digiflip_species_find(const char* key);
const DigiflipEvolutionRule*
    digiflip_evolution_match(DigiflipSpeciesId from, const DigiflipEvolutionContext* context);
uint32_t digiflip_stage_duration(DigiflipStage stage);
const char* digiflip_stage_name(DigiflipStage stage);
const char* digiflip_attribute_name(DigiflipAttribute attribute);
/* Vaccine > Virus > Data > Vaccine; Free has no advantage either way. */
bool digiflip_attribute_advantage(DigiflipAttribute attacker, DigiflipAttribute defender);
const DigiflipColosseumOpponent* digiflip_colosseum_opponent(uint8_t round);
const DigiflipTagRound* digiflip_colosseum_tag_round(uint8_t round);
/* Any opponent (single or tag) with this Colosseum-only boss index. */
const DigiflipColosseumOpponent* digiflip_colosseum_boss(uint8_t boss);
