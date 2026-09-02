// Drehencoder EC1 (GPIO8 = A, GPIO7 = B). Kein Taster im Knopf - jedes
// Bestaetigen laeuft ueber Touch.
//
// Warum die Auswertung so aussieht (am Geraet gemessen):
// Der Geber rastet bei A=B=1 und schnappt pro Rastung durch einen kompletten
// Quadraturzyklus. Das geht so schnell, dass die ISR von 3->2->0->1->3 nur EINE
// Zwischenstufe zu sehen bekommt - der Mitschnitt zeigt ausschliesslich 3->2->3
// und 3->1->3, nie die 0. Eine klassische Flankensumme hebt sich damit exakt auf
// (-1 und +1) und der Ring waere tot.
// Also: die **erste Zwischenstufe nach dem Verlassen der Ruhelage** bestimmt die
// Richtung, gezaehlt wird bei der Rueckkehr in die Ruhelage. Das funktioniert
// beim schnellen Schnappen genauso wie beim langsamen Durchdrehen (dort ist die
// erste Zwischenstufe dieselbe).
// Dazu eine Sperrzeit: die Kontakte prellen, ein Klick liefert im Mitschnitt
// mehrere komplette Zyklen innerhalb derselben Millisekunde - und die Prell-
// zyklen melden abwechselnd die Gegenrichtung. Nach einer gezaehlten Rastung
// werden Flanken deshalb kurz ignoriert (der Zustandsautomat laeuft mit).
#include <Arduino.h>
#include <soc/gpio_struct.h>
#include <esp_timer.h>
#include "board_pins.h"
#include "input_knob.h"

#if KNOB_QUADRATURE
// ============================ Waveshare-Knob: echte Quadratur =================

#define DEBOUNCE_US 12000
#define REVERSAL_MS 80                    // Gegenschritt kurz nach einer Drehung =
                                          // Nachprellen beim Einrasten, kein Wechsel                  // Sperrzeit nach einer Rastung (Menschen
                                          // schaffen keine 125 Klicks pro Sekunde)

static volatile int32_t s_detents = 0;    // gezaehlte Rastungen
static volatile int64_t s_last_step_us = 0;
static volatile int32_t s_edges = 0;      // jede erkannte Flanke (Diagnose)
static volatile int8_t  s_pending = 0;    // Richtung der laufenden Rastung
static volatile uint8_t s_state = 3;
static uint8_t s_rest = 3;                // Pegel in der Ruhelage, beim Start gemessen

static int32_t s_consumed = 0;
static volatile int64_t s_prev_step_us = 0;
static volatile uint8_t s_fast_run = 0;   // Zahl der schnell aufeinanderfolgenden Rastungen
static volatile int8_t  s_last_dir = 0;

// Diagnose: Ringpuffer der Flanken, wird im Hauptloop ausgegeben
#define TRACE_N 64
static volatile uint32_t s_trace[TRACE_N];
static volatile uint8_t s_tr_w = 0;
static volatile uint8_t s_tr_r = 0;

static inline uint8_t read_ab() {
  uint32_t in = GPIO.in;                  // Registerzugriff statt digitalRead (IRAM-sicher)
  return (uint8_t)((((in >> EC1_A) & 1) << 1) | ((in >> EC1_B) & 1));
}

static void IRAM_ATTR isr_enc() {
  uint8_t s = read_ab();
  uint8_t prev = s_state;
  if (s == prev) return;
  s_state = s;
  s_edges++;

  int64_t now = esp_timer_get_time();
  if (now - s_last_step_us < DEBOUNCE_US) { s_pending = 0; return; }   // Prellen

  if (prev == s_rest) {                   // Ruhelage verlassen -> Richtung steht fest
    s_pending = (s == KNOB_FIRST_CW) ? 1 : -1;
  } else if (s == s_rest && s_pending) {  // wieder eingerastet -> Schritt zaehlt
    uint32_t gap_ms = (uint32_t)((now - s_prev_step_us) / 1000);
    if (s_last_dir && s_pending != s_last_dir && gap_ms < REVERSAL_MS) {
      s_pending = 0;                      // Nachprellen, keine echte Gegendrehung
      return;
    }
    s_detents += s_pending;
    s_last_dir = s_pending;
    s_pending = 0;
    // Drehgeschwindigkeit hier messen, nicht beim Abholen: der Hauptloop haengt
    // waehrend eines Repaints und wuerde die Abstaende verfaelschen.
    s_prev_step_us = now;
    if (gap_ms < 70) { if (s_fast_run < 40) s_fast_run++; } else s_fast_run = 0;
    s_last_step_us = now;
  }

  if (s == s_rest && !s_pending) {        // gezaehlte Rastung protokollieren
    uint8_t w = s_tr_w, nxt = (uint8_t)((w + 1) % TRACE_N);
    if (nxt != s_tr_r) {                  // voll -> lieber verwerfen als ueberschreiben
      s_trace[w] = ((uint32_t)(now / 1000) << 8) | (uint8_t)(int8_t)(s_detents & 0xFF);
      s_tr_w = nxt;
    }
  }
}

void knob_init() {
  pinMode(EC1_A, INPUT_PULLUP);
  pinMode(EC1_B, INPUT_PULLUP);
  delay(2);
  s_state = read_ab();
  s_rest  = s_state;                      // im Ruhezustand steht der Geber auf einer Rastung
  attachInterrupt(digitalPinToInterrupt(EC1_A), isr_enc, CHANGE);
  attachInterrupt(digitalPinToInterrupt(EC1_B), isr_enc, CHANGE);
  Serial.printf("[knob] EC1 bereit, Ruhelage AB=%d\n", s_rest);
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
int knob_pin_state()   { return read_ab(); }

void knob_dump_trace() {
  if (!Serial) { s_tr_r = s_tr_w; return; }   // ohne Zuhoerer verwerfen, nicht blockieren
  while (s_tr_r != s_tr_w) {
    uint32_t e = s_trace[s_tr_r];
    s_tr_r = (uint8_t)((s_tr_r + 1) % TRACE_N);
    Serial.printf("[step] t=%lu rast=%d flanken=%ld\n",
                  (unsigned long)(e >> 8), (int)(int8_t)(e & 0xFF), (long)s_edges);
  }
}

// Beschleunigung erst nach anhaltendem schnellem Drehen - sonst springt die
// Anzeige schon bei normalem Bedienen in Fuenferschritten.
int knob_accel_step() {
  if (s_fast_run >= 14) return 10;
  if (s_fast_run >= 6)  return 5;
  return 1;
}

#else
// ============================ Guition K718: KEINE Quadratur ===================
// Der Knopf liefert pro Rastung genau einen sauberen LOW-Impuls, und zwar auf
// unterschiedlichen Leitungen je Drehrichtung: links auf KNOB_PIN_LEFT, rechts
// auf KNOB_PIN_RIGHT. Ein Quadraturdekoder (auch der PCNT im Chip) liest hier
// schlicht null. Also zwei Interrupts auf fallende Flanke, fertig.

#define PULSE_DEBOUNCE_US 4000

static volatile int32_t s_detents = 0;
static volatile int32_t s_edges = 0;
static volatile int64_t s_last_step_us = 0;
static volatile int64_t s_prev_step_us = 0;
static volatile uint8_t s_fast_run = 0;
static int32_t s_consumed = 0;

static void IRAM_ATTR count(int dir) {
  int64_t now = esp_timer_get_time();
  s_edges++;
  if (now - s_last_step_us < PULSE_DEBOUNCE_US) return;   // Prellen
  s_last_step_us = now;

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
void knob_dump_trace() {}

int knob_accel_step() {
  if (s_fast_run >= 14) return 10;
  if (s_fast_run >= 6)  return 5;
  return 1;
}

#endif  // KNOB_QUADRATURE
