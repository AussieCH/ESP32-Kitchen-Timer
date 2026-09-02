// Spannungsmessung mit Ueberabtastung und gleitendem Mittel - der Rohwert
// zappelt sonst um mehrere Zehntelvolt (kein Kondensator am Teiler).
#include <Arduino.h>
#include "board_pins.h"
#include "battery.h"

#define SAMPLES   32
#define EMA_ALPHA 0.15f

static float s_volts = 0.0f;

// Kennlinie Li-Ion. Bewusst grob: das ist eine Heuristik, keine Messung des
// Ladezustands - pro Geraet nachjustieren, wenn es genauer sein soll.
static const struct { float v; int pct; } CURVE[] = {
  { 4.20f, 100 }, { 4.10f, 92 }, { 4.00f, 82 }, { 3.90f, 68 }, { 3.80f, 54 },
  { 3.72f, 40 },  { 3.66f, 28 }, { 3.60f, 18 }, { 3.52f, 10 }, { 3.42f, 4 },
  { 3.30f, 0 },
};

void battery_init() {
  // Reihenfolge zaehlt: der Arduino-Core 3.x richtet den ADC-Kanal erst beim
  // ersten analogRead ein. Vorher gesetzte Daempfung laeuft ins Leere und der
  // Core meldet "Pin is not configured as analog channel".
  pinMode(BATT_ADC, INPUT);
  analogRead(BATT_ADC);
  analogSetPinAttenuation(BATT_ADC, ADC_11db);

  for (int i = 0; i < 8; i++) battery_tick();   // Mittelwert vorfuellen
  Serial.printf("[batt] %.2f V (%s)\n", battery_volts(),
                battery_is_cell() ? "Zelle" : "USB/Rail");
}

void battery_tick() {
  uint32_t sum = 0;
  for (int i = 0; i < SAMPLES; i++) sum += analogReadMilliVolts(BATT_ADC);
  float v = (sum / (float)SAMPLES) / 1000.0f * 2.0f;   // Teiler 10k/10k
  s_volts = s_volts == 0.0f ? v : s_volts + EMA_ALPHA * (v - s_volts);
}

float battery_volts() { return s_volts; }

// Ueber der Ladeschlussspannung ist es kein Ruhezustand mehr: entweder haengt
// das Geraet am Ladegeraet oder wir messen ein Rail (Waveshare: 5V-Schiene).
// Beides taugt nicht fuer eine Prozentzahl - dann lieber nur die Spannung zeigen.
bool battery_is_cell() { return s_volts > 2.5f && s_volts < 4.25f; }

int battery_percent() {
  if (!battery_is_cell()) return -1;
  const int n = sizeof(CURVE) / sizeof(CURVE[0]);
  if (s_volts >= CURVE[0].v) return 100;
  for (int i = 1; i < n; i++) {
    if (s_volts >= CURVE[i].v) {
      float t = (s_volts - CURVE[i].v) / (CURVE[i - 1].v - CURVE[i].v);
      return (int)(CURVE[i].pct + t * (CURVE[i - 1].pct - CURVE[i].pct));
    }
  }
  return 0;
}

bool battery_low() { int p = battery_percent(); return p >= 0 && p < 15; }
