// CST816 (0x15) als Minimal-Poller. Bewusst ohne Fremdbibliothek: die
// kursierenden Libs schreiben Controller-Register um und legen den Touch lahm
// (dasselbe Muster wie beim GT911 auf dem CrowPanel).
#include <Arduino.h>
#include <Wire.h>
#include "board_pins.h"
#include "input_touch.h"

static bool s_present = false;

static bool reg_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(CST816_ADDR);
  Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}

void touch_init() {
  pinMode(TP_RST, OUTPUT);
  digitalWrite(TP_RST, LOW);  delay(10);
  digitalWrite(TP_RST, HIGH); delay(60);
  pinMode(TP_INT, INPUT_PULLUP);

  Wire.begin(TP_SDA, TP_SCL, 400000);

  Wire.beginTransmission(CST816_ADDR);
  s_present = (Wire.endTransmission() == 0);
  if (!s_present) { Serial.println("[touch] CST816 nicht gefunden"); return; }

  reg_write(0xFE, 0x01);   // DisAutoSleep - sonst antwortet der Chip im Polling nicht mehr
  Serial.println("[touch] CST816 ok");
}

bool touch_read(int16_t *x, int16_t *y) {
  if (!s_present) return false;

  Wire.beginTransmission(CST816_ADDR);
  Wire.write(0x02);                       // FingerNum, XH, XL, YH, YL
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)CST816_ADDR, (uint8_t)5) != 5) return false;

  uint8_t b[5];
  for (int i = 0; i < 5; i++) b[i] = Wire.read();
  if ((b[0] & 0x0F) == 0) return false;

  int16_t px = ((b[1] & 0x0F) << 8) | b[2];
  int16_t py = ((b[3] & 0x0F) << 8) | b[4];

#if TP_SWAP_XY
  int16_t t = px; px = py; py = t;
#endif
#if TP_MIRROR_X
  px = 359 - px;
#endif
#if TP_MIRROR_Y
  py = 359 - py;
#endif
  if (px < 0) px = 0; if (px > 359) px = 359;
  if (py < 0) py = 0; if (py > 359) py = 359;
  *x = px; *y = py;
  return true;
}
