// WS2812-Ring (13 LEDs, GRB) auf dem Guition-Board.
//
// Zwei Aufgaben: im Betrieb zeigt er die Restzeit des naechsten Timers als
// Bogen - man sieht auch bei dunklem Display, dass etwas laeuft - und im Alarm
// blinkt er im selben Takt wie die Anzeige.
//
// GPIO0 ist zugleich BOOT-Strappingpin. Wir treiben ihn erst in leds_init(),
// also lange nach dem Reset.
#include "leds.h"

#if HAS_LEDRING

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define RING_MAX_BRIGHT   90    // ungedimmt blendet der Ring in einer dunklen Kueche
#define RING_IDLE_BRIGHT  35

static Adafruit_NeoPixel s_ring(LED_RING_COUNT, LED_RING_PIN, NEO_GRB + NEO_KHZ800);
static bool s_on = false;

void leds_init() {
  s_ring.begin();
  s_ring.setBrightness(RING_IDLE_BRIGHT);
  s_ring.clear();
  s_ring.show();
}

bool leds_present() { return true; }

void leds_off() {
  if (!s_on) return;
  s_on = false;
  s_ring.clear();
  s_ring.show();
}

void leds_progress(uint32_t rgb, float frac) {
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  s_ring.setBrightness(RING_IDLE_BRIGHT);

  float lit = frac * LED_RING_COUNT;          // wieviele LEDs die Restzeit fuellt
  for (int i = 0; i < LED_RING_COUNT; i++) {
    float part = lit - i;                     // 1 = ganz an, 0..1 = anteilig
    if (part <= 0) { s_ring.setPixelColor(i, 0); continue; }
    if (part > 1) part = 1;
    uint8_t r = (uint8_t)(((rgb >> 16) & 0xFF) * part);
    uint8_t g = (uint8_t)(((rgb >> 8)  & 0xFF) * part);
    uint8_t b = (uint8_t)(( rgb        & 0xFF) * part);
    s_ring.setPixelColor(i, r, g, b);
  }
  s_ring.show();
  s_on = true;
}

void leds_alarm_phase(uint32_t rgb, bool on) {
  s_ring.setBrightness(on ? RING_MAX_BRIGHT : 0);
  uint32_t c = on ? rgb : 0;
  for (int i = 0; i < LED_RING_COUNT; i++) s_ring.setPixelColor(i, c);
  s_ring.show();
  s_on = true;
}

#else   // Board ohne Ring

void leds_init() {}
void leds_progress(uint32_t, float) {}
void leds_alarm_phase(uint32_t, bool) {}
void leds_off() {}
bool leds_present() { return false; }

#endif
