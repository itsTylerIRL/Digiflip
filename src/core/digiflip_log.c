#include "digiflip_log.h"

#include <stdio.h>
#include <string.h>

void digiflip_log_clear(DigiflipLog* log) {
    memset(log, 0, sizeof(*log));
}

void digiflip_log_push(DigiflipLog* log, const DigiflipEvent* event) {
    if(log->head >= DIGIFLIP_LOG_CAPACITY) log->head = 0;
    log->events[log->head] = *event;
    log->head = (uint16_t)((log->head + 1U) % DIGIFLIP_LOG_CAPACITY);
    if(log->count < DIGIFLIP_LOG_CAPACITY) log->count++;
}

const DigiflipEvent* digiflip_log_get(const DigiflipLog* log, uint16_t index) {
    if(index >= log->count || log->count > DIGIFLIP_LOG_CAPACITY) return NULL;
    const uint16_t slot =
        (uint16_t)((log->head + DIGIFLIP_LOG_CAPACITY - 1U - index) % DIGIFLIP_LOG_CAPACITY);
    return &log->events[slot];
}

static const char* digiflip_species_label(uint16_t id) {
    const DigiflipSpecies* species = digiflip_species_get(id);
    return species ? species->name : "?";
}

/* Mirrors DigiflipCallReason (1 hunger, 2 strength, 4 tired). */
static const char* digiflip_call_label(uint8_t reason) {
    switch(reason) {
    case 1:
        return "hungry";
    case 2:
        return "weak";
    case 4:
        return "sleepy";
    default:
        return "?";
    }
}

/* Mirrors DigiflipDeathCause. */
static const char* digiflip_death_label(uint8_t cause) {
    static const char* const labels[] = {
        "?", "20 injuries", "20 mistakes", "neglect", "injured 6h", "starved 12h"};
    return cause < sizeof(labels) / sizeof(labels[0]) ? labels[cause] : "?";
}

static const char* digiflip_cheat_label(uint8_t cheat) {
    switch(cheat) {
    case DigiflipCheatFreezeHearts:
        return "hearts";
    case DigiflipCheatNoWaste:
        return "poop";
    case DigiflipCheatFillHearts:
        return "fill hearts";
    case DigiflipCheatForceEvolution:
        return "force evolution";
    case DigiflipCheatAllEggs:
        return "all eggs";
    default:
        return "?";
    }
}

/* Mirrors the DigiflipEgg names. */
static const char* digiflip_egg_label(uint8_t egg) {
    return egg < DigiflipEggCount ? digiflip_eggs[egg].name : "?";
}

void digiflip_event_describe(const DigiflipEvent* e, char* out, size_t size) {
    switch((DigiflipEventType)e->type) {
    case DigiflipEventNewEgg:
        snprintf(out, size, "New %segg: %s", e->b ? "traited " : "", digiflip_egg_label(e->a));
        break;
    case DigiflipEventHatched:
        snprintf(out, size, "Hatched %s", digiflip_species_label(e->b));
        break;
    case DigiflipEventEvolved:
        if(e->a) {
            snprintf(out, size, "Evolved to %s (%u%%)", digiflip_species_label(e->c), e->a);
        } else {
            snprintf(out, size, "Evolved to %s", digiflip_species_label(e->c));
        }
        break;
    case DigiflipEventNoEvolution:
        if(e->a) {
            snprintf(out, size, "Missed Stage V (%u%%)", e->a);
        } else {
            snprintf(out, size, "No evolution route");
        }
        break;
    case DigiflipEventCall:
        snprintf(out, size, "Call: %s", digiflip_call_label(e->a));
        break;
    case DigiflipEventCallAnswered:
        snprintf(out, size, "Answered: %s", digiflip_call_label(e->a));
        break;
    case DigiflipEventCareMistake:
        snprintf(out, size, "Mistake #%u (%s)", e->b, digiflip_call_label(e->a));
        break;
    case DigiflipEventHungerEmpty:
        snprintf(out, size, "Hunger empty");
        break;
    case DigiflipEventStrengthEmpty:
        snprintf(out, size, "Strength empty");
        break;
    case DigiflipEventPooped:
        snprintf(out, size, "Pooped (%u piles)", e->a);
        break;
    case DigiflipEventInjured:
        snprintf(
            out, size, "Injured #%u (%s)", e->b, e->a == DigiflipInjuryBattle ? "battle" : "waste");
        break;
    case DigiflipEventMedicine:
        snprintf(out, size, "Medicine: %s", e->a ? "healed" : "no effect");
        break;
    case DigiflipEventFed:
        snprintf(
            out,
            size,
            "%s %s",
            e->b ? "Overfed" : "Fed",
            e->a == DigiflipFoodProtein ? "protein" : "meat");
        break;
    case DigiflipEventTrained:
        if(e->c) {
            snprintf(out, size, "Tag training %u/5%s", e->b, e->a ? " OK" : "");
        } else {
            snprintf(out, size, "Trained: %u hits%s", e->b, e->a ? " OK" : "");
        }
        break;
    case DigiflipEventBattle:
        snprintf(out, size, "%s %sround %u", e->a ? "Won" : "Lost", e->c ? "tag " : "", e->b);
        break;
    case DigiflipEventCleaned:
        snprintf(out, size, "Cleaned %u piles", e->a);
        break;
    case DigiflipEventTired:
        snprintf(out, size, "Got sleepy");
        break;
    case DigiflipEventSlept:
        snprintf(
            out, size, e->b ? "Fell asleep, lights on" : (e->a ? "Went to bed" : "Took a nap"));
        break;
    case DigiflipEventWoke:
        snprintf(out, size, e->a ? "Woke up" : "Woken up");
        break;
    case DigiflipEventDied:
        snprintf(out, size, "Died: %s", digiflip_death_label(e->a));
        break;
    case DigiflipEventCheat:
        if(e->a == DigiflipCheatFreezeHearts || e->a == DigiflipCheatNoWaste ||
           e->a == DigiflipCheatAllEggs) {
            /* "Cheat: hearts frozen", "Cheat: poop off" and back. */
            const bool hearts = e->a == DigiflipCheatFreezeHearts;
            snprintf(
                out,
                size,
                "Cheat: %s %s",
                digiflip_cheat_label(e->a),
                hearts                       ? (e->b ? "frozen" : "normal") :
                e->a == DigiflipCheatNoWaste ? (e->b ? "off" : "on") :
                                               (e->b ? "open" : "earned"));
        } else {
            snprintf(out, size, "Cheat: %s", digiflip_cheat_label(e->a));
        }
        break;
    case DigiflipEventLightsOff:
        snprintf(out, size, "Lights off");
        break;
    case DigiflipEventEggUnlocked:
        snprintf(out, size, "Unlocked %s egg", digiflip_egg_label(e->a));
        break;
    case DigiflipEventNone:
    case DigiflipEventCount:
    default:
        snprintf(out, size, "?");
        break;
    }
}

/* Days since 1970-01-01 to a civil date. */
static void digiflip_civil(uint32_t time, int* year, unsigned* month, unsigned* day) {
    const long z = (long)(time / 86400U) + 719468L;
    const long era = z / 146097L;
    const unsigned doe = (unsigned)(z - era * 146097L);
    const unsigned yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    const unsigned doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    const unsigned mp = (5U * doy + 2U) / 153U;
    *day = doy - (153U * mp + 2U) / 5U + 1U;
    *month = mp < 10U ? mp + 3U : mp - 9U;
    *year = (int)(yoe + era * 400L + (*month <= 2U ? 1 : 0));
}

void digiflip_time_short(uint32_t time, char* out, size_t size) {
    int year;
    unsigned month, day;
    digiflip_civil(time, &year, &month, &day);
    const unsigned seconds = time % 86400U;
    snprintf(out, size, "%02u/%02u %02u:%02u", month, day, seconds / 3600U, (seconds / 60U) % 60U);
}

void digiflip_time_long(uint32_t time, char* out, size_t size) {
    int year;
    unsigned month, day;
    digiflip_civil(time, &year, &month, &day);
    const unsigned seconds = time % 86400U;
    snprintf(
        out,
        size,
        "%04d-%02u-%02u %02u:%02u:%02u",
        year,
        month,
        day,
        seconds / 3600U,
        (seconds / 60U) % 60U,
        seconds % 60U);
}
