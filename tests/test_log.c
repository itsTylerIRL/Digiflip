#include "../src/core/digiflip_log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    DigiflipLog log;
    digiflip_log_clear(&log);
    assert(digiflip_log_get(&log, 0) == NULL);

    /* Newest first, and the ring keeps only the latest CAPACITY events. */
    for(uint32_t i = 0; i < DIGIFLIP_LOG_CAPACITY + 10U; i++) {
        DigiflipEvent e = {.time = 1000U + i, .type = DigiflipEventPooped, .a = (uint8_t)(i % 4U)};
        digiflip_log_push(&log, &e);
    }
    assert(log.count == DIGIFLIP_LOG_CAPACITY);
    assert(digiflip_log_get(&log, 0)->time == 1000U + DIGIFLIP_LOG_CAPACITY + 9U);
    assert(digiflip_log_get(&log, DIGIFLIP_LOG_CAPACITY - 1U)->time == 1010U);
    assert(digiflip_log_get(&log, DIGIFLIP_LOG_CAPACITY) == NULL);

    char text[48];
    const DigiflipEvent evolved = {
        .type = DigiflipEventEvolved,
        .b = digiflip_species_find("koro"),
        .c = digiflip_species_find("agu")};
    digiflip_event_describe(&evolved, text, sizeof(text));
    assert(strcmp(text, "Evolved to Agumon") == 0);
    const DigiflipEvent mistake = {.type = DigiflipEventCareMistake, .a = 1, .b = 3};
    digiflip_event_describe(&mistake, text, sizeof(text));
    assert(strcmp(text, "Mistake #3 (hungry)") == 0);
    const DigiflipEvent battle = {.type = DigiflipEventBattle, .a = 1, .b = 7, .c = 1};
    digiflip_event_describe(&battle, text, sizeof(text));
    assert(strcmp(text, "Won tag round 7") == 0);
    const DigiflipEvent cheat = {.type = DigiflipEventCheat, .a = DigiflipCheatNoWaste, .b = 1};
    digiflip_event_describe(&cheat, text, sizeof(text));
    assert(strcmp(text, "Cheat: poop off") == 0); /* no-poop cheat switched on */

    /* 2026-09-27 13:05:09 */
    digiflip_time_long(1790514309U, text, sizeof(text));
    assert(strcmp(text, "2026-09-27 13:05:09") == 0);
    digiflip_time_short(1790514309U, text, sizeof(text));
    assert(strcmp(text, "09/27 13:05") == 0);
    digiflip_time_long(951782400U, text, sizeof(text)); /* leap day */
    assert(strcmp(text, "2000-02-29 00:00:00") == 0);

    puts("digiflip log tests passed");
    return 0;
}
