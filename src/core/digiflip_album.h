#pragma once

#include "digiflip_roster.h"

#include <stdbool.h>
#include <stdint.h>

/* Every Digimon ever raised (hatched or evolved into), across all eggs.
   Room for 256 species so the roster can grow without a format change. */
#define DIGIFLIP_ALBUM_CAPACITY 256U
#define DIGIFLIP_ALBUM_BYTES    (DIGIFLIP_ALBUM_CAPACITY / 8U)

typedef struct {
    uint8_t seen[DIGIFLIP_ALBUM_BYTES];
} DigiflipAlbum;

void digiflip_album_clear(DigiflipAlbum* album);
/* Returns true when the species is new to the album. */
bool digiflip_album_mark(DigiflipAlbum* album, DigiflipSpeciesId species);
bool digiflip_album_has(const DigiflipAlbum* album, DigiflipSpeciesId species);
/* Entries among the current roster. */
uint16_t digiflip_album_count(const DigiflipAlbum* album);
/* The next raised species after (step > 0) or before (step < 0) `from`,
   wrapping around the roster. Returns `from` when it's the only entry, and
   DIGIFLIP_SPECIES_NONE when the album is empty. */
DigiflipSpeciesId
    digiflip_album_step(const DigiflipAlbum* album, DigiflipSpeciesId from, int step);
