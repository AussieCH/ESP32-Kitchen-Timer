// Screen: Stoppuhr (hochzaehlend).
// Laeuft im Hintergrund weiter, auch wenn man wegwischt oder das Display
// schlafen geht - die Zeit haengt an derselben monotonen Basis wie die Timer.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"

static lv_obj_t *s_big, *s_tenth, *s_btn_run, *s_btn_reset, *s_hint;
static lv_timer_t *s_fast = nullptr;

static bool s_running = false;
static int64_t s_started_ms = 0;
static uint32_t s_carry_ms = 0;      // Summe frueherer Laeufe

static uint32_t elapsed_ms() {
  return s_running ? s_carry_ms + (uint32_t)(now_ms() - s_started_ms) : s_carry_ms;
}

static void paint(bool force) {
  uint32_t ms = elapsed_ms();
  static uint32_t last_s = 0xFFFFFFFF;
  uint32_t sec = ms / 1000;

  if (force || sec != last_s) {
    last_s = sec;
    char buf[16];
    if (sec >= 3600) snprintf(buf, sizeof(buf), "%u:%02u:%02u", sec / 3600, (sec / 60) % 60, sec % 60);
    else             snprintf(buf, sizeof(buf), "%02u:%02u", sec / 60, sec % 60);
    lv_obj_set_style_text_font(s_big, sec >= 3600 ? &font_time_52 : &font_time_92, 0);
    lv_label_set_text(s_big, buf);
  }
  lv_label_set_text_fmt(s_tenth, ".%u", (ms / 100) % 10);
}

static void fast_cb(lv_timer_t *) { paint(false); }

static void run_cb(lv_event_t *) {
  if (s_running) {
    s_carry_ms = elapsed_ms();
    s_running = false;
    lv_timer_pause(s_fast);
  } else {
    s_started_ms = now_ms();
    s_running = true;
    lv_timer_resume(s_fast);
  }
  haptic_bump();
  ui_stopwatch_update();
}

static void reset_cb(lv_event_t *) {
  s_running = false;
  s_carry_ms = 0;
  if (s_fast) lv_timer_pause(s_fast);
  haptic_bump();
  paint(true);
  ui_stopwatch_update();
}

void ui_stopwatch_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Stoppuhr");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -150);

  s_big = lv_label_create(p);
  lv_obj_set_style_text_font(s_big, &font_time_92, 0);
  lv_obj_align(s_big, LV_ALIGN_CENTER, 0, -30);

  s_tenth = lv_label_create(p);
  lv_obj_set_style_text_font(s_tenth, &font_time_30, 0);
  lv_obj_set_style_text_color(s_tenth, lv_palette_main(LV_PALETTE_AMBER), 0);
  lv_obj_align(s_tenth, LV_ALIGN_CENTER, 0, 30);

  s_btn_run = make_button(p, LV_SYMBOL_PLAY " Start", 112, 54, run_cb, nullptr);
  lv_obj_align(s_btn_run, LV_ALIGN_CENTER, -60, 99);

  s_btn_reset = make_button(p, "Zurueck", 112, 54, reset_cb, nullptr);
  lv_obj_align(s_btn_reset, LV_ALIGN_CENTER, 60, 99);

  s_hint = lv_label_create(p);
  lv_obj_set_style_text_font(s_hint, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_hint, col_dim(), 0);
  lv_obj_align(s_hint, LV_ALIGN_CENTER, 0, 140);

  s_fast = lv_timer_create(fast_cb, 100, nullptr);   // Zehntel brauchen 10 Hz
  lv_timer_pause(s_fast);
  paint(true);
}

void ui_stopwatch_update() {
  button_set_text(s_btn_run, s_running ? LV_SYMBOL_PAUSE " Pause" : LV_SYMBOL_PLAY " Start");
  if (s_running) button_set_color(s_btn_run, lv_palette_main(LV_PALETTE_AMBER));
  else {
    lv_obj_set_style_bg_color(s_btn_run, lv_color_hex(0x272B33), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(s_btn_run, 0), lv_color_white(), 0);
  }
  if (s_running || elapsed_ms()) lv_obj_clear_state(s_btn_reset, LV_STATE_DISABLED);
  else                           lv_obj_add_state(s_btn_reset, LV_STATE_DISABLED);
  lv_label_set_text(s_hint, s_running ? "laeuft im Hintergrund weiter" : "");
  paint(true);
}

bool ui_stopwatch_running() { return s_running; }
