// Haptik, Ton, LED-Ring und Backlight gibt es auf dem Host nicht - die UI ruft
// sie trotzdem. Der Simulator prueft Layout und Ablauf, nicht die Peripherie.
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "battery.h"
#include "meater.h"

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
void leds_single(int, uint32_t) {}
bool leds_present() { return false; }

void leds_countdown(float, uint32_t) {}

void  battery_init() {}
void  battery_tick() {}
float battery_volts() { return 3.92f; }
int   battery_percent() { return 71; }
bool  battery_is_cell() { return true; }
bool  battery_low() { return false; }

void meater_init() {}
static MeaterState s_sim_meater = MEATER_CONNECTED;
void sim_meater_state(MeaterState st) { s_sim_meater = st; }
MeaterState meater_state() { return s_sim_meater; }
float meater_tip_c() { return 62.4f; }
float meater_ambient_c() { return 178.0f; }
int meater_battery() { return 84; }
uint32_t meater_age_ms() { return s_sim_meater == MEATER_CONNECTED ? 1200 : 999999; }
const char *meater_name() { return "MEATER+"; }

void app_apply_brightness(int) {}
