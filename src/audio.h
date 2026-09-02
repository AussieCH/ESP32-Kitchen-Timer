#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "board_pins.h"
#include "melodies.h"

// Tonausgabe. Auf Boards ohne Lautsprecher sind alle Funktionen wirkungslos.
void audio_init();
void audio_set_volume(int vol);          // 0..100
int  audio_get_volume();
void audio_play(const Melody *m, bool repeat);
void audio_stop();
bool audio_is_playing();
void audio_set_fade_in(bool on);         // Alarm langsam lauter werden lassen
bool audio_present();
