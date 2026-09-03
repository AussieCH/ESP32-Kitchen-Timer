// Kuechentimer fuer den Waveshare ESP32-S3-Knob-Touch-LCD-1.8 (autark, kein WLAN).
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "lgfx_knob.h"
#include "board_pins.h"
#include "input_touch.h"
#include "input_knob.h"
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "battery.h"
#include "meater.h"
#include "timers.h"
#include "ui.h"

#define BUF_LINES 40
#define IDLE_MS   60000        // danach Backlight aus; laufende Timer laufen weiter

static LGFX_Knob lcd;
static lv_disp_draw_buf_t s_draw_buf;
static lv_color_t *s_buf1, *s_buf2;

static uint32_t s_last_input_ms = 0;
static bool s_awake = true;
static bool s_touch_blocked = false;   // die Beruehrung, die aufweckt, loest nichts aus

void app_apply_brightness(int pct) {
  if (s_awake) lcd.setBrightness(map(constrain(pct, 0, 100), 0, 100, 8, 255));
}

static void wake() {
  s_last_input_ms = millis();
  if (s_awake) return;
  s_awake = true;
  lcd.setBrightness(map(setting_brightness(), 0, 100, 8, 255));
}

static void doze() {
  if (!s_awake) return;
  s_awake = false;
  lcd.setBrightness(0);
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *px) {
  uint32_t w = area->x2 - area->x1 + 1, h = area->y2 - area->y1 + 1;
  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.writePixels((uint16_t *)px, w * h, true);   // true = Byteswap fuer LVGL-RGB565
  lcd.endWrite();
  lv_disp_flush_ready(drv);
}

static void touch_cb(lv_indev_drv_t *, lv_indev_data_t *data) {
  int16_t x = 0, y = 0;
  bool pressed = touch_read(&x, &y);

  if (pressed) {
    s_last_input_ms = millis();
    if (!s_awake) { wake(); s_touch_blocked = true; }
  } else {
    s_touch_blocked = false;
  }

  if (pressed && !s_touch_blocked) {
    data->point.x = x; data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  Serial.begin(115200);
  // Serial ist hier USB-CDC. Liest der Host nicht mit, laeuft dessen Sendepuffer
  // voll und jedes printf blockiert bis zum Timeout - der Hauptloop steht dann
  // still und das Geraet wirkt eingefroren. Timeout 0 = lieber verwerfen.
  Serial.setTxTimeoutMs(0);
  delay(200);
  Serial.printf("\n[boot] Küchentimer auf %s\n", BOARD_NAME);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(0);
  lcd.fillScreen(TFT_BLACK);

  lv_init();
  size_t px = SCR_W * BUF_LINES;
  s_buf1 = (lv_color_t *)heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  s_buf2 = (lv_color_t *)heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, px);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCR_W;
  disp_drv.ver_res = SCR_H;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&disp_drv);

  touch_init();     // startet den I2C-Bus, muss vor haptic_init laufen

  // Wer haengt am I2C-Bus? Beantwortet unter anderem, ob dieses Board wirklich
  // einen Haptiktreiber hat - Datenblaetter und Herstellerdemos widersprechen sich.
  Serial.print("[i2c] gefunden:");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); found++; }
  }
  Serial.println(found ? "" : " nichts");

  haptic_init();
  knob_init();
  audio_init();
  leds_init();
  battery_init();
  meater_init();          // Bluetooth-Suche nach dem Grill-Thermometer
  timers_init();    // liest Vorlagen + Lautstaerke/Helligkeit aus dem NVS

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touch_cb;
  lv_indev_drv_register(&indev_drv);

  ui_init();
  ui_splash_show();          // RONDO-Logo, laeuft im Hintergrund aus
  s_last_input_ms = millis();
  app_apply_brightness(setting_brightness());
  Serial.printf("[boot] frei: %u B intern, %u B PSRAM\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void loop() {
  lv_timer_handler();

  int d = knob_take_delta();
  if (d) {
    if (!s_awake) wake();            // erste Rastung weckt nur, verstellt nichts
    else { s_last_input_ms = millis(); ui_on_knob(d, knob_accel_step()); }
  }

  static uint32_t t_timers = 0, t_ui = 0;
  uint32_t now = millis();

  static uint32_t t_batt = 0;
  if (now - t_batt >= 1000) { t_batt = now; battery_tick(); }

  if (now - t_timers >= 100) {
    t_timers = now;
    if (timers_tick()) {             // ein Timer ist gerade abgelaufen
      wake();
      haptic_bump();
      ui_alarm_check();
    }
  }

  if (now - t_ui >= 250) {
    t_ui = now;
    ui_tick();
#ifdef DIAG_MODE
    lv_mem_monitor_t mem;
    lv_mem_monitor(&mem);
    char dbg[80];
    snprintf(dbg, sizeof(dbg), "rast=%ld t=%d f=%d lv=%uB/%u%%",
             (long)knob_detents(), ui_current_tile(), ui_new_focus(),
             (unsigned)mem.free_size, (unsigned)mem.used_pct);
    ui_debug_set(dbg);

    static uint32_t t_mem = 0;
    if (Serial && now - t_mem >= 2000) {   // nur schreiben, wenn wirklich jemand lauscht
      t_mem = now;
      Serial.printf("[mem] lvgl frei=%u frag=%u%% belegt=%u%% | intern=%u | loop=%lu ms\n",
                    (unsigned)mem.free_size, (unsigned)mem.frag_pct, (unsigned)mem.used_pct,
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                    (unsigned long)(millis() - now));
    }
#endif
  }

#ifdef DIAG_MODE
  knob_dump_trace();             // Flankenprotokoll, solange die Diagnose an ist
#endif

  // LED-Ring zeigt die Restzeit des naechsten Timers - auch bei dunklem Display
  // sieht man so, dass etwas laeuft. Im Alarm gehoert der Ring dem Blinktakt.
  static uint32_t t_ring = 0;
  if (leds_present() && !ui_splash_active() && now - t_ring >= 250) {
    t_ring = now;
    if (timer_first_ringing() < 0) {
      uint32_t rgb; float frac; uint32_t rest_s; bool stopwatch;
      if (ui_ring_source(&rgb, &frac, &rest_s, &stopwatch)) {
        // Stoppuhr hat keine Restzeit: statt eines Bogens wandert ein Punkt
        // im Sekundentakt - das liest sich von weitem als "laeuft".
        if (stopwatch) leds_single((int)(rest_s % LED_RING_COUNT), rgb);
        else           leds_timer(rgb, frac, rest_s);
      } else {
        leds_off();
      }
    }
  }

  if (s_awake && now - s_last_input_ms > IDLE_MS && timer_first_ringing() < 0) doze();

  delay(2);
}
