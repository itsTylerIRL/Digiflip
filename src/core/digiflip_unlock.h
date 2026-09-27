#pragma once

#include "digiflip_album.h"
#include "digiflip_roster.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DigiflipUnlockDefault,
    DigiflipUnlockChild, /* any Digimon raised to Child */
    DigiflipUnlockWins, /* battles won across every pet */
    DigiflipUnlockAlbum, /* different Digimon in the album */
    DigiflipUnlockLink, /* a device link on the original; open here */
} DigiflipUnlockKind;

typedef struct {
    DigiflipUnlockKind kind;
    bool unlocked;
    uint16_t progress;
    uint16_t goal;
} DigiflipEggStatus;

DigiflipEggStatus digiflip_egg_status(DigiflipEgg egg, const DigiflipAlbum* album, uint16_t wins);
/* Bit n set when egg n can be picked. */
uint16_t digiflip_eggs_unlocked(const DigiflipAlbum* album, uint16_t wins);
