// Screen: laufende Timer - und zugleich der Alarmzustand.
//
// Durchgeschaltet wird mit dem Drehring oder hoch/runter wischen. Zur Liste
// gehoert auch die **Stoppuhr**, sobald sie laeuft; sie hat die Kennung 0,
// echte Timer beginnen bei 1.
//
// Die Farbe eines Timers ist seine Identitaet: sie faerbt Ring, Icon und
// Fortschrittsbogen auf dem Schirm - und den LED-Ring, sobald der Timer
// ausgewaehlt ist (siehe ui_ring_source).
//
// Alarm: dieser Screen kommt nach vorn, die Anzeige blinkt in der Timer-Farbe,
// Haptik, LED-Ring und Melodie laufen im selben Takt mit.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "melodies.h"

#define SEL_STOPWATCH 0u          // Kennung der Stoppuhr in der Auswahl
#define FLASH_MS 130

static lv_obj_t *s_arc, *s_idx, *s_icon, *s_time, *s_sub;
static lv_obj_t *s_btn_pause, *s_btn_plus, *s_btn_del;
static lv_obj_t *s_empty, *s_flash, *s_stop_area;
static lv_timer_t *s_flash_timer = nullptr;
static bool s_flash_on = false;

static uint32_t s_sel_id = 0;
static uint32_t s_pending_id = 0;
static uint32_t s_paint_sig = 0xFFFFFFFF;

// ---------------------------------------------------------------- Auswahl
static bool stopwatch_listed() { return ui_stopwatch_running(); }
static int  entry_count() { return active_count() + (stopwatch_listed() ? 1 : 0); }
static bool sel_is_stopwatch() { return s_sel_id == SEL_STOPWATCH && stopwatch_listed(); }

// Index des ausgewaehlten Timers in der Timerliste, -1 wenn keiner
static int sel_timer_index() {
  if (s_sel_id == SEL_STOPWATCH) return -1;
  int n = active_count();
  for (int i = 0; i < n; i++) if (active_at(i)->id == s_sel_id) return i;
  return -1;
}

// Position in der Gesamtliste (Timer zuerst, Stoppuhr zuletzt), -1 wenn leer
static int sel_pos() {
  if (sel_is_stopwatch()) return active_count();
  int i = sel_timer_index();
  if (i >= 0) return i;
  if (active_count() > 0) { s_sel_id = active_at(0)->id; return 0; }
  if (stopwatch_listed()) { s_sel_id = SEL_STOPWATCH; return 0; }
  return -1;
}

static void select_pos(int pos) {
  int n = active_count();
  s_sel_id = (pos < n) ? active_at(pos)->id : SEL_STOPWATCH;
  s_paint_sig = 0xFFFFFFFF;
}

// ---------------------------------------------------------------- Blinken
static void flash_stop() {
  haptic_buzz(false);
  audio_stop();
  leds_off();
  s_flash_on = false;
  lv_obj_set_style_bg_opa(s_flash, LV_OPA_0, 0);
  if (s_flash_timer) lv_timer_pause(s_flash_timer);
}

static void flash_cb(lv_timer_t *) {
  int idx = timer_first_ringing();
  if (idx < 0) { flash_stop(); return; }

  lv_color_t c = timer_color(active_at(idx)->color);
  s_flash_on = !s_flash_on;
  lv_obj_set_style_bg_color(s_flash, c, 0);
  lv_obj_set_style_bg_opa(s_flash, s_flash_on ? LV_OPA_40 : LV_OPA_0, 0);
  haptic_buzz(s_flash_on);
  leds_alarm_phase(lv_color_to32(c) & 0xFFFFFF, s_flash_on);
}

void ui_alarm_check() {
  int idx = timer_first_ringing();
  if (idx < 0) { flash_stop(); return; }

  ActiveTimer *t = active_at(idx);
  s_sel_id = t->id;                       // der Alarm bestimmt, was man sieht
  s_paint_sig = 0xFFFFFFFF;
  if (!audio_is_playing()) {
    audio_set_fade_in(true);
    audio_play(&MELODIES[t->melody % MELODY_COUNT], true);
  }
  ui_goto(TILE_ACTIVE);
  ui_active_update();
  if (!s_flash_timer) s_flash_timer = lv_timer_create(flash_cb, FLASH_MS, nullptr);
  lv_timer_resume(s_flash_timer);
}

// ---------------------------------------------------------------- Aktionen
static void del_yes(void *) {
  for (int i = 0; i < active_count(); i++)
    if (active_at(i)->id == s_pending_id) { timer_delete(i); break; }
  s_paint_sig = 0xFFFFFFFF;
  ui_toast("Timer geloescht");
  ui_active_update();
}

static void del_cb(lv_event_t *) {
  int i = sel_timer_index();
  if (i < 0) return;
  s_pending_id = active_at(i)->id;
  ui_confirm("Timer loeschen?", del_yes, nullptr);
}

static void stop_alarm(int idx) {
  timer_ack(idx);
  flash_stop();
  haptic_bump();
  s_paint_sig = 0xFFFFFFFF;
  ui_active_update();
}

static void pause_cb(lv_event_t *) {
  if (sel_is_stopwatch()) { ui_stopwatch_toggle(); s_paint_sig = 0xFFFFFFFF; ui_active_update(); return; }
  int i = sel_timer_index();
  if (i < 0) return;
  ActiveTimer *t = active_at(i);
  if (t->ringing || t->expired) { stop_alarm(i); return; }   // Button ist dann "Stopp"
  timer_toggle_pause(i);
  s_paint_sig = 0xFFFFFFFF;
  ui_active_update();
}

static void plus_cb(lv_event_t *) {
  if (sel_is_stopwatch()) { ui_stopwatch_reset(); s_paint_sig = 0xFFFFFFFF; ui_active_update(); return; }
  int i = sel_timer_index();
  if (i < 0) return;
  ActiveTimer *t = active_at(i);
  if (t->ringing || t->expired) {          // aus dem Alarm heraus verlaengern
    timer_snooze(i, 5 * 60);
    flash_stop();
    ui_toast("+5 Minuten");
  } else {
    timer_add_seconds(i, 60);
    ui_toast("+1 Minute");
  }
  s_paint_sig = 0xFFFFFFFF;
  ui_active_update();
}

static void stoparea_cb(lv_event_t *) {
  int i = sel_timer_index();
  if (i < 0) return;
  ActiveTimer *t = active_at(i);
  if (t->ringing || t->expired) stop_alarm(i);
}

static void newtimer_cb(lv_event_t *) { ui_goto(TILE_NEW); }

// ---------------------------------------------------------------- Aufbau
void ui_active_create(lv_obj_t *p) {
  s_arc = lv_arc_create(p);
  lv_obj_set_size(s_arc, 352, 352);
  lv_obj_center(s_arc);
  lv_arc_set_rotation(s_arc, 270);
  lv_arc_set_bg_angles(s_arc, 0, 360);
  lv_arc_set_range(s_arc, 0, 1000);
  lv_obj_remove_style(s_arc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(s_arc, 12, LV_PART_MAIN);
  lv_obj_set_style_arc_width(s_arc, 12, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(s_arc, lv_color_hex(0x23272F), LV_PART_MAIN);

  s_icon = icon_create(p, 64);
  lv_obj_align(s_icon, LV_ALIGN_CENTER, 0, -84);

  s_time = lv_label_create(p);
  lv_obj_set_style_text_font(s_time, &font_time_92, 0);
  lv_obj_align(s_time, LV_ALIGN_CENTER, 0, -6);

  s_sub = lv_label_create(p);
  lv_obj_set_style_text_font(s_sub, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_sub, col_dim(), 0);
  lv_obj_align(s_sub, LV_ALIGN_CENTER, 0, 52);

  s_btn_pause = make_button(p, LV_SYMBOL_PAUSE " Pause", 112, 54, pause_cb, nullptr);
  lv_obj_align(s_btn_pause, LV_ALIGN_CENTER, -60, 99);

  s_btn_plus = make_button(p, "+1 Min", 112, 54, plus_cb, nullptr);
  lv_obj_align(s_btn_plus, LV_ALIGN_CENTER, 60, 99);

  s_idx = lv_label_create(p);
  lv_obj_set_style_text_font(s_idx, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_idx, col_dim(), 0);
  lv_obj_align(s_idx, LV_ALIGN_CENTER, 0, 136);

  s_btn_del = make_button(p, LV_SYMBOL_CLOSE, 44, 44, del_cb, nullptr);
  lv_obj_align(s_btn_del, LV_ALIGN_CENTER, 0, -152);

  // Leerzustand: nicht einfach leer, sondern der Weg zum naechsten Timer
  s_empty = lv_obj_create(p);
  lv_obj_set_size(s_empty, SCR_W, SCR_H);
  lv_obj_center(s_empty);
  lv_obj_set_style_bg_color(s_empty, lv_color_black(), 0);
  lv_obj_set_style_border_width(s_empty, 0, 0);
  lv_obj_set_style_radius(s_empty, LV_RADIUS_CIRCLE, 0);
  lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *el = lv_label_create(s_empty);
  lv_label_set_text(el, "Kein Timer laeuft");
  lv_obj_set_style_text_font(el, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(el, col_dim(), 0);
  lv_obj_align(el, LV_ALIGN_CENTER, 0, -40);
  lv_obj_t *eb = make_button(s_empty, "Neuer Timer", 180, 58, newtimer_cb, nullptr);
  lv_obj_align(eb, LV_ALIGN_CENTER, 0, 30);
  button_set_color(eb, lv_palette_main(LV_PALETTE_AMBER));

  // Blinkflaeche liegt ueber allem, faengt aber keine Beruehrungen ab
  s_flash = lv_obj_create(p);
  lv_obj_set_size(s_flash, SCR_W, SCR_H);
  lv_obj_center(s_flash);
  lv_obj_set_style_radius(s_flash, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(s_flash, 0, 0);
  lv_obj_set_style_bg_opa(s_flash, LV_OPA_0, 0);
  lv_obj_clear_flag(s_flash, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  // Im Alarm ist die halbe Bildmitte die Stopptaste - man tippt mit dem
  // Handruecken, nicht mit der Fingerspitze.
  s_stop_area = lv_obj_create(p);
  lv_obj_set_size(s_stop_area, 260, 210);
  lv_obj_align(s_stop_area, LV_ALIGN_CENTER, 0, -40);
  lv_obj_set_style_bg_opa(s_stop_area, LV_OPA_0, 0);
  lv_obj_set_style_border_width(s_stop_area, 0, 0);
  lv_obj_clear_flag(s_stop_area, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_stop_area, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(s_stop_area, stoparea_cb, LV_EVENT_CLICKED, nullptr);
}

void ui_active_focus_id(uint32_t id) { s_sel_id = id; s_paint_sig = 0xFFFFFFFF; }
void ui_active_focus_stopwatch()     { s_sel_id = SEL_STOPWATCH; s_paint_sig = 0xFFFFFFFF; }

// ---------------------------------------------------------------- Anzeige
static void show_stopwatch(int pos, int n) {
  uint32_t ms = ui_stopwatch_elapsed_ms();
  uint32_t sec = ms / 1000;
  bool run = ui_stopwatch_running() && ms > 0;

  uint32_t sig = 0x5709u + sec * 131u + (run ? 1u : 0u) + (uint32_t)n * 65537u;
  if (sig == s_paint_sig) return;
  s_paint_sig = sig;

  lv_obj_add_flag(s_arc, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_icon, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_btn_del, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s_stop_area, LV_OBJ_FLAG_HIDDEN);

  char buf[16];
  if (sec >= 3600) snprintf(buf, sizeof(buf), "%u:%02u:%02u", sec / 3600, (sec / 60) % 60, sec % 60);
  else             snprintf(buf, sizeof(buf), "%02u:%02u", sec / 60, sec % 60);
  lv_obj_set_style_text_font(s_time, sec >= 3600 ? &font_time_52 : &font_time_92, 0);
  lv_label_set_text(s_time, buf);
  lv_obj_set_style_text_color(s_time, lv_color_white(), 0);

  lv_label_set_text(s_sub, ui_stopwatch_running() ? "Stoppuhr" : "Stoppuhr  -  pausiert");
  lv_label_set_text_fmt(s_idx, n > 1 ? "%d / %d" : "", pos + 1, n);

  button_set_text(s_btn_pause, ui_stopwatch_running() ? LV_SYMBOL_PAUSE " Pause"
                                                      : LV_SYMBOL_PLAY " Weiter");
  lv_obj_set_style_bg_color(s_btn_pause, lv_color_hex(0x272B33), 0);
  lv_obj_set_style_text_color(lv_obj_get_child(s_btn_pause, 0), lv_color_white(), 0);
  button_set_text(s_btn_plus, "Zurueck");
}

void ui_active_update() {
  int n = entry_count();
  int pos = sel_pos();

  if (pos < 0) {
    if (lv_obj_has_flag(s_empty, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(s_empty);
    }
    lv_obj_add_flag(s_stop_area, LV_OBJ_FLAG_HIDDEN);
    s_paint_sig = 0xFFFFFFFF;
    return;
  }
  lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);

  if (sel_is_stopwatch()) { show_stopwatch(pos, n); return; }

  ActiveTimer *t = active_at(sel_timer_index());
  uint32_t rest_ms = timer_remaining_ms(t);
  uint32_t rest_s = (rest_ms + 999) / 1000;
  bool alarm = t->ringing || t->expired;

  // Nur neu zeichnen, wenn sich sichtbar etwas geaendert hat - ein Vollbild-
  // Repaint kostet auf dem QSPI-Panel spuerbar Zeit.
  uint32_t sig = rest_s * 131u + t->id * 7919u + (uint32_t)n * 65537u
               + (t->paused ? 1u : 0u) + (t->ringing ? 2u : 0u) + (t->expired ? 4u : 0u);
  if (sig == s_paint_sig) return;
  s_paint_sig = sig;

  lv_color_t c = timer_color(t->color);
  lv_obj_clear_flag(s_icon, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s_btn_del, LV_OBJ_FLAG_HIDDEN);

  char buf[16];
  fmt_time(buf, sizeof(buf), rest_s);
  lv_obj_set_style_text_font(s_time, rest_s >= 3600 ? &font_time_52 : &font_time_92, 0);
  lv_label_set_text(s_time, buf);
  lv_obj_set_style_text_color(s_time, alarm ? c : (t->paused ? col_dim() : lv_color_white()), 0);

  lv_obj_set_style_arc_color(s_arc, c, LV_PART_INDICATOR);
  uint32_t total_ms = t->total_s * 1000UL;
  int arc_val = total_ms ? (int)((int64_t)rest_ms * 1000 / total_ms) : 0;
  // Einen Bogen der Laenge null nie zeichnen lassen: lv_draw_mask_radius_init
  // haengt sich daran auf. Im Alarm hat er ohnehin nichts mehr anzuzeigen.
  if (arc_val > 0) {
    lv_obj_clear_flag(s_arc, LV_OBJ_FLAG_HIDDEN);
    lv_arc_set_value(s_arc, arc_val);
  } else {
    lv_obj_add_flag(s_arc, LV_OBJ_FLAG_HIDDEN);
  }

  icon_set(s_icon, t->icon, c);
  lv_label_set_text_fmt(s_idx, n > 1 ? "%d / %d" : "", pos + 1, n);

  char tot[16]; fmt_time(tot, sizeof(tot), t->total_s);
  if (alarm)          lv_label_set_text_fmt(s_sub, "%s abgelaufen", icon_name(t->icon));
  else if (t->paused) lv_label_set_text_fmt(s_sub, "pausiert  -  %s  %s", tot, icon_name(t->icon));
  else                lv_label_set_text_fmt(s_sub, "%s  %s", tot, icon_name(t->icon));

  if (alarm) {
    button_set_text(s_btn_pause, LV_SYMBOL_STOP " Stopp");
    button_set_color(s_btn_pause, c);
    button_set_text(s_btn_plus, "+5 Min");
    lv_obj_clear_flag(s_stop_area, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_stop_area);
  } else {
    button_set_text(s_btn_pause, t->paused ? LV_SYMBOL_PLAY " Weiter" : LV_SYMBOL_PAUSE " Pause");
    lv_obj_set_style_bg_color(s_btn_pause, lv_color_hex(0x272B33), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(s_btn_pause, 0), lv_color_white(), 0);
    button_set_text(s_btn_plus, "+1 Min");
    lv_obj_add_flag(s_stop_area, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_clear_state(s_btn_pause, LV_STATE_DISABLED);
}

// Der LED-Ring folgt der Auswahl, nicht dem ersten Timer.
bool ui_ring_source(uint32_t *rgb, float *frac, uint32_t *rest_s, bool *stopwatch) {
  int pos = sel_pos();
  if (pos < 0) return false;

  if (sel_is_stopwatch()) {
    *rgb = 0xFDF4E9;                       // Creme wie im Logo - kein Timer, keine Farbe
    *frac = 0.0f;
    *rest_s = ui_stopwatch_elapsed_ms() / 1000;
    *stopwatch = true;
    return ui_stopwatch_running();
  }
  ActiveTimer *t = active_at(sel_timer_index());
  if (!t || t->expired) return false;
  uint32_t total = t->total_s * 1000UL;
  uint32_t rest = timer_remaining_ms(t);
  *rgb = lv_color_to32(timer_color(t->color)) & 0xFFFFFF;
  *frac = total ? (float)rest / total : 0.0f;
  *rest_s = (rest + 999) / 1000;
  *stopwatch = false;
  return true;
}

static void move_sel(int step) {
  int n = entry_count();
  if (n < 2) return;
  int pos = sel_pos();
  pos = (pos + step % n + n) % n;
  select_pos(pos);
  haptic_click();
  ui_active_update();
}

void ui_active_gesture(lv_dir_t dir) { move_sel(dir == LV_DIR_TOP ? 1 : -1); }
void ui_active_knob(int d, int)      { move_sel(d); }
