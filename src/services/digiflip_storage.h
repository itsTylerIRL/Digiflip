#pragma once

#include "../core/digiflip_album.h"
#include "../core/digiflip_game.h"
#include "../core/digiflip_log.h"

/* Slot 0 is save.bin (the first pet), slot 1 is save2.bin (the second). */
bool digiflip_storage_load_slot(DigiflipGame* game, uint8_t slot);
bool digiflip_storage_save_slot(const DigiflipGame* game, uint8_t slot);

/* Player preferences, kept in their own file so they survive new eggs and
   save-format changes. */
typedef struct {
    bool sound; /* tones (still subject to the Flipper's volume) */
    bool call_light; /* blink the LED while the pet is calling */
    /* Cheats (see DigiflipRules). */
    bool cheat_freeze_hearts;
    bool cheat_no_waste;
    bool cheat_all_eggs;
} DigiflipSettings;

void digiflip_settings_defaults(DigiflipSettings* settings);
bool digiflip_settings_load(DigiflipSettings* settings);
bool digiflip_settings_save(const DigiflipSettings* settings);

/* The album of every Digimon raised, kept apart from the pet save so it
   survives new eggs. */
bool digiflip_album_load(DigiflipAlbum* album);
bool digiflip_album_save(const DigiflipAlbum* album);

/* Device-wide progress that outlives every pet. */
typedef struct {
    uint16_t wins; /* battles won by any pet */
    uint16_t known_eggs; /* unlocked eggs already announced */
} DigiflipRecords;

bool digiflip_records_load(DigiflipRecords* records);
bool digiflip_records_save(const DigiflipRecords* records);

/* The pet's event log, persisted across sessions. */
bool digiflip_log_load(DigiflipLog* log);
bool digiflip_log_save(const DigiflipLog* log);
/* Write the log, oldest first, to apps_data/digiflip/log.txt for bug reports. */
bool digiflip_log_export(const DigiflipLog* log);

/* The second slot holds nothing, a second raised pet (save2.bin), or a
   Copymon: a copy of a raised Digimon that needs no care. */
typedef enum {
    DigiflipSlotEmpty,
    DigiflipSlotPet,
    DigiflipSlotCopymon,
} DigiflipSlotKind;

bool digiflip_slot2_load(DigiflipSlotKind* kind, DigiflipSpeciesId* copymon);
bool digiflip_slot2_save(DigiflipSlotKind kind, DigiflipSpeciesId copymon);
