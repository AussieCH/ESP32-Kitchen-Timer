// Drehknopf des Guition JC3636K718C. Kein Taster im Knopf - jedes Bestaetigen
// laeuft ueber Touch.
//
// Das ist KEIN Quadraturgeber: ein PCNT-Encoder liest hier null. Pro Rastung
// kommt ein LOW-Impuls, und zwar je nach Drehrichtung auf einer anderen
// Leitung. Am Geraet mitgeschnitten kamen zwei Eigenheiten dazu:
//   * es sind ZWEI Impulse pro Rastung (67-125 ms auseinander, der erste auf
//     halbem Weg) - ungeteilt springt die Anzeige beim halben Klick,
//   * bei jeder Rastung klingelt die GEGENLEITUNG nach: ein Schwall in
//     derselben Millisekunde plus einzelne Nachzuegler noch nach ~50 ms. Die
//     zaehlten als Gegenschritt und verwarfen die angefangene Rastung - das
//     fuehlte sich als Haken beim Drehen an.
#include <Arduino.h>
#include <esp_timer.h>
#include "board_pins.h"
#include "input_knob.h"
#define PULSE_DEBOUNCE_US 4000
// Am Geraet gemessen: bei jeder Rastung klingelt auch die GEGENLEITUNG nach.
// Der Schwall direkt danach faellt in die Sperre oben, einzelne Nachzuegler
// kamen aber noch nach 48 ms - und die zaehlten dann als Gegenschritt und
// verwarfen die angefangene Rastung. Deshalb: Richtungswechsel erst nach einer
// Pause akzeptieren. Eine echte Gegendrehung braucht laenger als das.
#define REVERSE_LOCK_US 150000

static volatile int32_t s_detents = 0;
static volatile int32_t s_edges = 0;
static volatile int8_t  s_acc = 0;        // Impulse seit der letzten gezaehlten Rastung
static volatile int64_t s_last_pulse_us = 0;
static volatile int64_t s_prev_step_us = 0;
static volatile uint8_t s_fast_run = 0;
static volatile int8_t  s_dir_lock = 0;   // Richtung des letzten akzeptierten Impulses
static volatile int64_t s_last_acc_us = 0;
static int32_t s_consumed = 0;

// Diagnose: jeder Rohimpuls mit Zeit, Leitung und ob er gezaehlt wurde
#define PTRACE_N 96
static volatile uint32_t s_ptrace[PTRACE_N];
static volatile uint8_t s_pw = 0;
static volatile uint8_t s_pr = 0;

static inline void IRAM_ATTR ptrace(int64_t now, int dir, int accepted) {
  uint8_t w = s_pw, nxt = (uint8_t)((w + 1) % PTRACE_N);
  if (nxt == s_pr) return;                    // voll: verwerfen statt luegen
  s_ptrace[w] = ((uint32_t)(now / 1000) << 2) | (dir > 0 ? 1 : 0) | (accepted ? 2 : 0);
  s_pw = nxt;
}

static void IRAM_ATTR count(int dir) {
  int64_t now = esp_timer_get_time();
  s_edges++;
  if (now - s_last_pulse_us < PULSE_DEBOUNCE_US) { ptrace(now, dir, 0); return; }   // Prellen
  if (s_dir_lock && dir != s_dir_lock && now - s_last_acc_us < REVERSE_LOCK_US) {
    ptrace(now, dir, 0);                  // Nachklingeln der Gegenleitung
    return;
  }
  ptrace(now, dir, 1);
  s_last_pulse_us = now;
  s_last_acc_us = now;
  s_dir_lock = (int8_t)dir;

  // Richtungswechsel: angefangene Rastung verwerfen, sonst schluckt der Teiler
  // den ersten Schritt in die neue Richtung.
  if (s_acc && ((s_acc > 0) != (dir > 0))) s_acc = 0;
  s_acc += dir;
  if (s_acc != KNOB_PULSES_PER_DETENT && s_acc != -KNOB_PULSES_PER_DETENT) return;
  s_acc = 0;

  s_detents += dir;
  uint32_t gap_ms = (uint32_t)((now - s_prev_step_us) / 1000);
  s_prev_step_us = now;
  if (gap_ms < 70) { if (s_fast_run < 40) s_fast_run++; } else s_fast_run = 0;
}

static void IRAM_ATTR isr_left()  { count(-1); }
static void IRAM_ATTR isr_right() { count(+1); }

void knob_init() {
  pinMode(KNOB_PIN_LEFT,  INPUT_PULLUP);
  pinMode(KNOB_PIN_RIGHT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KNOB_PIN_LEFT),  isr_left,  FALLING);
  attachInterrupt(digitalPinToInterrupt(KNOB_PIN_RIGHT), isr_right, FALLING);
  Serial.println("[knob] Impulsgeber bereit (links GPIO" + String(KNOB_PIN_LEFT)
                 + ", rechts GPIO" + String(KNOB_PIN_RIGHT) + ")");
}

int knob_take_delta() {
  int32_t d = s_detents - s_consumed;
  if (d == 0) return 0;
  s_consumed += d;
#if KNOB_INVERT
  d = -d;
#endif
  return (int)d;
}

int32_t knob_raw()     { return s_edges; }
int32_t knob_detents() { return s_detents; }
int knob_pin_state()   { return (digitalRead(KNOB_PIN_LEFT) << 1) | digitalRead(KNOB_PIN_RIGHT); }

void knob_dump_trace() {
  if (!Serial) { s_pr = s_pw; return; }       // ohne Zuhoerer verwerfen, nicht blockieren
  while (s_pr != s_pw) {
    uint32_t e = s_ptrace[s_pr];
    s_pr = (uint8_t)((s_pr + 1) % PTRACE_N);
    Serial.printf("[p] t=%lu %s %s rast=%ld\n", (unsigned long)(e >> 2),
                  (e & 1) ? "RECHTS" : "links ", (e & 2) ? "gezaehlt " : "verworfen",
                  (long)s_detents);
  }
}

int knob_accel_step() {
  if (s_fast_run >= 14) return 10;
  if (s_fast_run >= 6)  return 5;
  return 1;
}
