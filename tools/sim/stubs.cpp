// Haptik, Ton, LED-Ring und Backlight gibt es auf dem Host nicht - die UI ruft
// sie trotzdem. Der Simulator prueft Layout und Ablauf, nicht die Peripherie.
#include "haptics.h"
#include "audio.h"
#include "leds.h"

void haptic_init() {}
void haptic_click() {}
void haptic_bump() {}
void haptic_buzz(bool) {}
bool haptic_present() { return true; }

static int s_vol = 70;
void audio_init() {}
void audio_set_volume(int v) { s_vol = v < 0 ? 0 : (v > 100 ? 100 : v); }
int  audio_get_volume() { return s_vol; }
void audio_play(const Melody *, bool) {}
void audio_stop() {}
bool audio_is_playing() { return false; }
void audio_set_fade_in(bool) {}
bool audio_present() { return true; }

void leds_init() {}
void leds_progress(uint32_t, float) {}
void leds_alarm_phase(uint32_t, bool) {}
void leds_off() {}
bool leds_present() { return false; }

void app_apply_brightness(int) {}
