#include "digiflip_unlock.h"

static bool digiflip_album_has_child(const DigiflipAlbum* album) {
    for(DigiflipSpeciesId id = 0; id < digiflip_species_count; id++) {
        if(digiflip_album_has(album, id) && digiflip_species[id].stage >= DigiflipStageChild) {
            return true;
        }
    }
    return false;
}

DigiflipEggStatus digiflip_egg_status(DigiflipEgg egg, const DigiflipAlbum* album, uint16_t wins) {
    DigiflipEggStatus status = {.kind = DigiflipUnlockDefault, .unlocked = true};
    switch(egg) {
    case DigiflipEggVersion2:
    case DigiflipEggVersion3:
    case DigiflipEggVersion4:
    case DigiflipEggVersion5:
        status.kind = DigiflipUnlockChild;
        status.goal = 1;
        status.progress = digiflip_album_has_child(album) ? 1U : 0U;
        break;
    case DigiflipEggZuba:
    case DigiflipEggHack:
        status.kind = DigiflipUnlockWins;
        status.goal = egg == DigiflipEggZuba ? 50U : 100U;
        status.progress = wins;
        break;
    case DigiflipEggSlayerdra:
    case DigiflipEggBreakdra:
        status.kind = DigiflipUnlockAlbum;
        status.goal = egg == DigiflipEggSlayerdra ? 5U : 25U;
        status.progress = digiflip_album_count(album);
        break;
    case DigiflipEggCorona:
    case DigiflipEggLuna:
    case DigiflipEggTaichi:
    case DigiflipEggYamato:
    case DigiflipEggDoru:
    case DigiflipEggMeicoo:
        status.kind = DigiflipUnlockLink;
        break;
    default:
        break;
    }
    if(status.goal) {
        if(status.progress > status.goal) status.progress = status.goal;
        status.unlocked = status.progress >= status.goal;
    }
    return status;
}

uint16_t digiflip_eggs_unlocked(const DigiflipAlbum* album, uint16_t wins) {
    uint16_t mask = 0;
    for(uint8_t egg = 0; egg < DigiflipEggCount; egg++) {
        if(digiflip_egg_status((DigiflipEgg)egg, album, wins).unlocked) {
            mask |= (uint16_t)(1U << egg);
        }
    }
    return mask;
}
