#include "digiflip_sound.h"

#include <notification/notification_messages.h>

static const NotificationSequence digiflip_sequence_click = {
    &message_click,
    &message_delay_25,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_feed = {
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_e5,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_happy = {
    &message_note_e5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_g5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_c6,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_success = {
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_g5,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_failure = {
    &message_note_e5,
    &message_delay_100,
    &message_sound_off,
    &message_delay_25,
    &message_note_c5,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_victory = {
    &message_note_g5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_e6,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_clean = {
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_g6,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_light_on = {
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_c6,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_light_off = {
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_c5,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_hatch = {
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_e5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_g5,
    &message_delay_50,
    &message_sound_off,
    &message_delay_25,
    &message_note_c6,
    &message_delay_100,
    &message_sound_off,
    NULL,
};

/* Three sharp beeps: the unmistakable "come look at me" call. */
static const NotificationSequence digiflip_sequence_call = {
    &message_note_a6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_note_a6,
    &message_delay_50,
    &message_sound_off,
    &message_delay_50,
    &message_note_a6,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_hit = {
    &message_note_c7,
    &message_delay_25,
    &message_sound_off,
    NULL,
};

static const NotificationSequence digiflip_sequence_death = {
    &message_note_g5,
    &message_delay_250,
    &message_sound_off,
    &message_delay_50,
    &message_note_e5,
    &message_delay_250,
    &message_sound_off,
    &message_delay_50,
    &message_note_c5,
    &message_delay_500,
    &message_sound_off,
    NULL,
};

static bool digiflip_sound_enabled = true;

void digiflip_sound_set_enabled(bool enabled) {
    digiflip_sound_enabled = enabled;
}

void digiflip_sound_play(NotificationApp* notification, DigiflipSound sound) {
    if(!digiflip_sound_enabled) return;
    const NotificationSequence* sequence = &digiflip_sequence_click;
    switch(sound) {
    case DigiflipSoundFeed:
        sequence = &digiflip_sequence_feed;
        break;
    case DigiflipSoundHappy:
        sequence = &digiflip_sequence_happy;
        break;
    case DigiflipSoundSuccess:
        sequence = &digiflip_sequence_success;
        break;
    case DigiflipSoundFailure:
        sequence = &digiflip_sequence_failure;
        break;
    case DigiflipSoundVictory:
        sequence = &digiflip_sequence_victory;
        break;
    case DigiflipSoundClean:
        sequence = &digiflip_sequence_clean;
        break;
    case DigiflipSoundLightOn:
        sequence = &digiflip_sequence_light_on;
        break;
    case DigiflipSoundLightOff:
        sequence = &digiflip_sequence_light_off;
        break;
    case DigiflipSoundHatch:
        sequence = &digiflip_sequence_hatch;
        break;
    case DigiflipSoundCall:
        sequence = &digiflip_sequence_call;
        break;
    case DigiflipSoundHit:
        sequence = &digiflip_sequence_hit;
        break;
    case DigiflipSoundDeath:
        sequence = &digiflip_sequence_death;
        break;
    case DigiflipSoundClick:
    default:
        break;
    }
    notification_message(notification, sequence);
}

void digiflip_call_led(NotificationApp* notification, bool on) {
    notification_message(notification, on ? &sequence_blink_start_red : &sequence_blink_stop);
}
