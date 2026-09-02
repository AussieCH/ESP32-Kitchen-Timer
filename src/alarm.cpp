// Alarmwirkung: Vollflaechiges Blinken ueber allem, Haptik, Melodie in
// Schleife und blinkender LED-Ring - alles im selben 130-ms-Takt.
//
// Die Blinkflaeche liegt auf lv_layer_top und faengt bewusst KEINE
// Beruehrungen ab: welcher Knopf den Alarm beendet, entscheidet der Screen,
// der ihn ausgeloest hat.
#include <Arduino.h>
#include <lvgl.h>
#include "alarm.h"
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "melodies.h"

#define FLASH_MS   130
#define TIMEOUT_MS (5 * 60 * 1000)      // danach verstummt er von selbst

static lv_obj_t *s_flash = nullptr;
static lv_timer_t *s_timer = nullptr;
static bool s_on = false;
static uint32_t s_rgb = 0xFFA000;
static uint32_t s_started = 0;
static AlarmKind s_kind = ALARM_NONE;

static void tick(lv_timer_t *) {
  if (millis() - s_started > TIMEOUT_MS) { alarm_stop(); return; }
  s_on = !s_on;
  lv_obj_set_style_bg_color(s_flash, lv_color_hex(s_rgb), 0);
  lv_obj_set_style_bg_opa(s_flash, s_on ? LV_OPA_40 : LV_OPA_0, 0);
  haptic_buzz(s_on);
  leds_alarm_phase(s_rgb, s_on);
}

void alarm_init() {
  s_flash = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_flash, 360, 360);
  lv_obj_center(s_flash);
  lv_obj_set_style_radius(s_flash, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(s_flash, 0, 0);
  lv_obj_set_style_bg_opa(s_flash, LV_OPA_0, 0);
  lv_obj_clear_flag(s_flash, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  s_timer = lv_timer_create(tick, FLASH_MS, nullptr);
  lv_timer_pause(s_timer);
}

void alarm_start(uint32_t rgb, uint8_t melody, AlarmKind kind) {
  s_rgb = rgb;
  s_kind = kind;
  s_started = millis();
  if (!audio_is_playing()) {
    audio_set_fade_in(true);                       // nicht sofort volle Lautstaerke
    audio_play(&MELODIES[melody % MELODY_COUNT], true);
  }
  lv_obj_move_foreground(s_flash);
  lv_timer_resume(s_timer);
}

void alarm_stop() {
  if (s_kind == ALARM_NONE) return;
  s_kind = ALARM_NONE;
  s_on = false;
  haptic_buzz(false);
  audio_stop();
  leds_off();
  lv_obj_set_style_bg_opa(s_flash, LV_OPA_0, 0);
  lv_timer_pause(s_timer);
}

bool alarm_active()     { return s_kind != ALARM_NONE; }
AlarmKind alarm_kind()  { return s_kind; }
