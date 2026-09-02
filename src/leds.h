#pragma once
#include <stdint.h>
#include "board_pins.h"

// WS2812-Ring. Ohne Ring auf dem Board sind alle Funktionen wirkungslos.
void leds_init();
void leds_progress(uint32_t rgb, float frac);   // Restzeit als leuchtender Bogen
void leds_alarm_phase(uint32_t rgb, bool on);   // Blinken im Alarmtakt
void leds_off();
bool leds_present();
