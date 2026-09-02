#pragma once
#include <stdint.h>
#include "board_pins.h"

// WS2812-Ring. Ohne Ring auf dem Board sind alle Funktionen wirkungslos.
void leds_init();
void leds_progress(uint32_t rgb, float frac);   // Bogen in fester Farbe
// Bogen mit Farbverlauf gruen -> amber -> rot; in der letzten Minute pulsiert er.
void leds_countdown(float frac, uint32_t rest_s);
void leds_alarm_phase(uint32_t rgb, bool on);   // Blinken im Alarmtakt
void leds_off();
bool leds_present();
