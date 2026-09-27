#include "digiflip_album.h"

#include <string.h>

void digiflip_album_clear(DigiflipAlbum* album) {
    memset(album, 0, sizeof(*album));
}

bool digiflip_album_mark(DigiflipAlbum* album, DigiflipSpeciesId species) {
    if(species >= digiflip_species_count || species >= DIGIFLIP_ALBUM_CAPACITY) return false;
    const uint8_t bit = (uint8_t)(1U << (species % 8U));
    if(album->seen[species / 8U] & bit) return false;
    album->seen[species / 8U] |= bit;
    return true;
}

bool digiflip_album_has(const DigiflipAlbum* album, DigiflipSpeciesId species) {
    if(species >= digiflip_species_count || species >= DIGIFLIP_ALBUM_CAPACITY) return false;
    return (album->seen[species / 8U] & (1U << (species % 8U))) != 0;
}

uint16_t digiflip_album_count(const DigiflipAlbum* album) {
    uint16_t count = 0;
    for(DigiflipSpeciesId id = 0; id < digiflip_species_count && id < DIGIFLIP_ALBUM_CAPACITY;
        id++) {
        if(digiflip_album_has(album, id)) count++;
    }
    return count;
}

DigiflipSpeciesId
    digiflip_album_step(const DigiflipAlbum* album, DigiflipSpeciesId from, int step) {
    const int count = (int)digiflip_species_count;
    if(count <= 0) return DIGIFLIP_SPECIES_NONE;
    const int start = from < count ? (int)from : 0;
    const int dir = step < 0 ? -1 : 1;
    for(int i = 1; i <= count; i++) {
        const int id = ((start + dir * i) % count + count) % count;
        if(digiflip_album_has(album, (DigiflipSpeciesId)id)) return (DigiflipSpeciesId)id;
    }
    return DIGIFLIP_SPECIES_NONE;
}
