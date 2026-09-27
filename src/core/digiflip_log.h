#pragma once

#include "digiflip_roster.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A timestamped record of something that happened to the pet, including
   events replayed while the app was closed. Used for the Logs screen and
   for debugging the game rules. */
typedef enum {
    DigiflipEventNone,
    DigiflipEventNewEgg, /* a: egg, b: 1 = traited */
    DigiflipEventHatched, /* b: species */
    DigiflipEventEvolved, /* a: chance % when rolled (else 0), b: from, c: to species */
    DigiflipEventNoEvolution, /* a: chance % when rolled, b: species with no route */
    DigiflipEventCall, /* a: DigiflipCallReason */
    DigiflipEventCallAnswered, /* a: DigiflipCallReason */
    DigiflipEventCareMistake, /* a: DigiflipCallReason, b: total this stage */
    DigiflipEventHungerEmpty,
    DigiflipEventStrengthEmpty,
    DigiflipEventPooped, /* a: piles */
    DigiflipEventInjured, /* a: DigiflipInjuryCause, b: injuries this stage */
    DigiflipEventMedicine, /* a: 1 = healed */
    DigiflipEventFed, /* a: DigiflipFood, b: 1 = overfeed */
    DigiflipEventTrained, /* a: 1 = success, b: presses (tag: hits), c: 1 = tag */
    DigiflipEventBattle, /* a: 1 = won, b: round (1-based), c: 1 = tag */
    DigiflipEventCleaned, /* a: piles */
    DigiflipEventTired,
    DigiflipEventSlept, /* a: 1 = night sleep, 0 = nap; b: 1 = fell asleep on its own */
    DigiflipEventWoke, /* a: 1 = on its own */
    DigiflipEventDied, /* a: DigiflipDeathCause */
    DigiflipEventCheat, /* a: DigiflipCheat, b: 1 = on (toggles) */
    DigiflipEventLightsOff,
    DigiflipEventEggUnlocked, /* a: egg */
    DigiflipEventCount,
} DigiflipEventType;

typedef enum {
    DigiflipInjuryWaste,
    DigiflipInjuryBattle,
} DigiflipInjuryCause;

typedef enum {
    DigiflipFoodMeat,
    DigiflipFoodProtein,
} DigiflipFood;

typedef enum {
    DigiflipCheatFreezeHearts,
    DigiflipCheatNoWaste,
    DigiflipCheatFillHearts,
    DigiflipCheatForceEvolution,
    DigiflipCheatAllEggs,
} DigiflipCheat;

typedef struct {
    uint32_t time; /* RTC seconds (local wall time) */
    uint8_t type; /* DigiflipEventType */
    uint8_t a;
    uint16_t b;
    uint16_t c;
    uint16_t slot; /* which pet slot (0 or 1); set by the app */
} DigiflipEvent;

#define DIGIFLIP_LOG_CAPACITY 128U

typedef struct {
    DigiflipEvent events[DIGIFLIP_LOG_CAPACITY];
    uint16_t head; /* next slot to write */
    uint16_t count;
} DigiflipLog;

void digiflip_log_clear(DigiflipLog* log);
void digiflip_log_push(DigiflipLog* log, const DigiflipEvent* event);
/* index 0 is the newest event; NULL past the end. */
const DigiflipEvent* digiflip_log_get(const DigiflipLog* log, uint16_t index);
/* Short description for the on-screen list ("Evolved to Agumon"). */
void digiflip_event_describe(const DigiflipEvent* event, char* out, size_t size);
/* "MM/DD HH:MM" and "YYYY-MM-DD HH:MM:SS" for an RTC timestamp. */
void digiflip_time_short(uint32_t time, char* out, size_t size);
void digiflip_time_long(uint32_t time, char* out, size_t size);
