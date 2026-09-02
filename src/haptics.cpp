// DRV2605L am Touch-I2C-Bus (0x5A). IN/TRIG liegt fest auf GND, EN fest auf 3V3
// -> der Baustein laesst sich ausschliesslich per I2C ausloesen.
//
// Wichtig: auf diesem Board haengt laut Schaltplan ein **LRA** an PP1/PP2, kein
// ERM. Deshalb N_ERM_LRA=1, Bibliothek 6 (LRA) und offener Regelkreis - im
// geschlossenen Regelkreis will der Treiber eine Kalibrierung sehen und laeuft
// ohne sie unter Umstaenden gar nicht erst an.
#include <Arduino.h>
#include <Wire.h>
#include "board_pins.h"
#include "haptics.h"

#if HAS_HAPTIC

#define REG_MODE      0x01
#define REG_RTP       0x02
#define REG_LIBRARY   0x03
#define REG_WAVESEQ1  0x04
#define REG_WAVESEQ2  0x05
#define REG_GO        0x0C
#define REG_ODCLAMP   0x17
#define REG_FEEDBACK  0x1A
#define REG_CONTROL3  0x1D

#define MODE_INTERNAL 0x00
#define MODE_RTP      0x05

static bool s_present = false;
static uint8_t s_mode = 0xFF;
static uint32_t s_last_ms = 0;
static bool s_buzzing = false;

static void wr(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(DRV2605_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

static void set_mode(uint8_t m) {
  if (s_mode == m) return;
  s_mode = m;
  wr(REG_MODE, m);
}

void haptic_init() {
  Wire.beginTransmission(DRV2605_ADDR);
  s_present = (Wire.endTransmission() == 0);
  if (!s_present) { Serial.println("[haptic] DRV2605 nicht gefunden"); return; }

  wr(REG_MODE, MODE_INTERNAL);   s_mode = MODE_INTERNAL;
  wr(REG_FEEDBACK, 0xB6);        // N_ERM_LRA=1, Bremsfaktor 3, Loop-Gain mittel
  wr(REG_CONTROL3, 0xA1);        // LRA im offenen Regelkreis (ohne Kalibrierung zuverlaessiger)
  wr(REG_ODCLAMP, 0xC0);         // kraeftig genug, um durch das Metallgehaeuse zu kommen
  wr(REG_LIBRARY, 6);            // LRA-Bibliothek
  wr(REG_WAVESEQ2, 0x00);
  Serial.println("[haptic] DRV2605 ok (LRA)");
}

bool haptic_present() { return s_present; }

static void play(uint8_t effect, uint16_t min_gap_ms) {
  if (!s_present || s_buzzing) return;
  uint32_t now = millis();
  if (now - s_last_ms < min_gap_ms) return;   // sonst verschluckt der Treiber Pulse
  s_last_ms = now;
  set_mode(MODE_INTERNAL);
  wr(REG_WAVESEQ1, effect);
  wr(REG_WAVESEQ2, 0x00);
  wr(REG_GO, 0x01);
}

// Effekt 4 = "Sharp Click 100%" - genau das kurze Ticken, das ein Drehring
// haben will. 18 ms Sperre reicht: zwei Rastungen liegen nie enger zusammen.
void haptic_click() { play(4,  18); }
void haptic_bump()  { play(1,  60); }    // "Strong Click" fuer Bestaetigungen

// Dauervibration ueber Real-Time-Playback: Amplitude direkt schreiben, statt
// Einzeleffekte hintereinanderzuhaengen - das gibt das gleichmaessige Schnarren.
void haptic_buzz(bool on) {
  if (!s_present) return;
  if (on == s_buzzing) return;
  s_buzzing = on;
  if (on) {
    set_mode(MODE_RTP);
    wr(REG_RTP, 0x7F);           // Vollausschlag
  } else {
    wr(REG_RTP, 0x00);
    set_mode(MODE_INTERNAL);
  }
}

#else   // Board ohne Haptik

void haptic_init() {}
void haptic_click() {}
void haptic_bump() {}
void haptic_buzz(bool) {}
bool haptic_present() { return false; }

#endif
