#pragma once

#include <notification/notification.h>

typedef enum {
    DigiflipSoundClick,
    DigiflipSoundFeed,
    DigiflipSoundHappy,
    DigiflipSoundSuccess,
    DigiflipSoundFailure,
    DigiflipSoundVictory,
    DigiflipSoundClean,
    DigiflipSoundLightOn,
    DigiflipSoundLightOff,
    DigiflipSoundHatch,
    DigiflipSoundCall,
    DigiflipSoundHit,
    DigiflipSoundDeath,
} DigiflipSound;

/* Global mute from the Settings menu; while off, play() does nothing. */
void digiflip_sound_set_enabled(bool enabled);
void digiflip_sound_play(NotificationApp* notification, DigiflipSound sound);
/* Blink the red LED while the call light is on. */
void digiflip_call_led(NotificationApp* notification, bool on);
