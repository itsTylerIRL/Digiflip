#include "../src/core/digiflip_album.h"
#include "../src/core/digiflip_unlock.h"

#include <assert.h>
#include <stdio.h>

static void test_egg_unlocks(void) {
    DigiflipAlbum album;
    digiflip_album_clear(&album);
    const uint16_t links = (1U << DigiflipEggCorona) | (1U << DigiflipEggLuna) |
                           (1U << DigiflipEggTaichi) | (1U << DigiflipEggYamato) |
                           (1U << DigiflipEggDoru) | (1U << DigiflipEggMeicoo);
    assert(digiflip_eggs_unlocked(&album, 0) == ((1U << DigiflipEggVersion1) | links));

    /* A Baby doesn't count; any Child opens Ver.2-5. */
    digiflip_album_mark(&album, digiflip_species_find("koro"));
    assert(!(digiflip_eggs_unlocked(&album, 0) & (1U << DigiflipEggVersion2)));
    digiflip_album_mark(&album, digiflip_species_find("agu"));
    const uint16_t child = digiflip_eggs_unlocked(&album, 0);
    for(unsigned egg = DigiflipEggVersion2; egg <= DigiflipEggVersion5; egg++)
        assert(child & (1U << egg));

    DigiflipEggStatus zuba = digiflip_egg_status(DigiflipEggZuba, &album, 49);
    assert(!zuba.unlocked && zuba.progress == 49 && zuba.goal == 50);
    assert(digiflip_egg_status(DigiflipEggZuba, &album, 50).unlocked);
    assert(!digiflip_egg_status(DigiflipEggHack, &album, 99).unlocked);
    DigiflipEggStatus hack = digiflip_egg_status(DigiflipEggHack, &album, 500);
    assert(hack.unlocked && hack.progress == 100);

    assert(!digiflip_egg_status(DigiflipEggSlayerdra, &album, 0).unlocked);
    for(DigiflipSpeciesId id = 0; digiflip_album_count(&album) < 5; id++)
        digiflip_album_mark(&album, id);
    assert(digiflip_egg_status(DigiflipEggSlayerdra, &album, 0).unlocked);
    assert(!digiflip_egg_status(DigiflipEggBreakdra, &album, 0).unlocked);
    for(DigiflipSpeciesId id = 0; digiflip_album_count(&album) < 25; id++)
        digiflip_album_mark(&album, id);
    assert(digiflip_egg_status(DigiflipEggBreakdra, &album, 0).unlocked);
}

int main(void) {
    test_egg_unlocks();
    DigiflipAlbum album;
    digiflip_album_clear(&album);
    assert(digiflip_album_count(&album) == 0);

    const DigiflipSpeciesId agumon = digiflip_species_find("agu");
    assert(!digiflip_album_has(&album, agumon));
    assert(digiflip_album_mark(&album, agumon)); /* new */
    assert(!digiflip_album_mark(&album, agumon)); /* already there */
    assert(digiflip_album_has(&album, agumon));
    assert(digiflip_album_count(&album) == 1);

    /* The last species and out-of-range ids. */
    const DigiflipSpeciesId last = (DigiflipSpeciesId)(digiflip_species_count - 1U);
    assert(digiflip_album_mark(&album, last));
    assert(digiflip_album_count(&album) == 2);
    assert(!digiflip_album_mark(&album, DIGIFLIP_SPECIES_NONE));
    assert(!digiflip_album_has(&album, DIGIFLIP_SPECIES_NONE));
    assert(!digiflip_album_mark(&album, (DigiflipSpeciesId)digiflip_species_count));

    for(DigiflipSpeciesId id = 0; id < digiflip_species_count; id++)
        digiflip_album_mark(&album, id);
    assert(digiflip_album_count(&album) == digiflip_species_count);
    /* Stepping through owned entries only, wrapping both ways. */
    DigiflipAlbum owned;
    digiflip_album_clear(&owned);
    assert(digiflip_album_step(&owned, 0, 1) == DIGIFLIP_SPECIES_NONE); /* empty */
    const DigiflipSpeciesId a = 5, b = 40, c = 100;
    digiflip_album_mark(&owned, a);
    assert(digiflip_album_step(&owned, a, 1) == a); /* only entry */
    assert(digiflip_album_step(&owned, 0, 1) == a); /* from an unowned spot */
    digiflip_album_mark(&owned, b);
    digiflip_album_mark(&owned, c);
    assert(digiflip_album_step(&owned, a, 1) == b);
    assert(digiflip_album_step(&owned, b, 1) == c);
    assert(digiflip_album_step(&owned, c, 1) == a); /* wraps forward */
    assert(digiflip_album_step(&owned, a, -1) == c); /* wraps backward */
    assert(digiflip_album_step(&owned, 50, -1) == b); /* from between entries */
    assert(digiflip_album_step(&owned, 50, 1) == c);

    puts("digiflip album tests passed");
    return 0;
}
