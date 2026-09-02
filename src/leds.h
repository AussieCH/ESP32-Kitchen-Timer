#pragma once
#include <stdint.h>
#include "board_pins.h"

// WS2812-Ring. Ohne Ring auf dem Board sind alle Funktionen wirkungslos.
void leds_init();
void leds_progress(uint32_t rgb, float frac);   // Bogen in fester Farbe
// Restzeit als Bogen in der Farbe des Timers; in den letzten zehn Sekunden
// pulsiert er. Die Farbe bleibt die des Timers - sie ist seine Identitaet.
void leds_timer(uint32_t rgb, float frac, uint32_t rest_s);
void leds_alarm_phase(uint32_t rgb, bool on);   // Blinken im Alarmtakt
void leds_single(int idx, uint32_t rgb);        // genau eine LED (Startbild)
void leds_off();
bool leds_present();
