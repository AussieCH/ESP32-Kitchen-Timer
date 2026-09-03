// Screen 2: neuen Timer anlegen.
// Segment antippen -> Drehring stellt es. Schnelles Drehen springt in 5er- und
// 10er-Schritten, sonst dauert eine Dreiviertelstunde ewig.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"
#include "audio.h"
#include "melodies.h"

#define F_H 0
#define F_M 1
#define F_S 2
#define F_ICON   3
#define F_MELODY 4

static lv_obj_t *s_lbl[3], *s_sep[2], *s_row;
static lv_obj_t *s_btn_icon, *s_icon_prev, *s_btn_start;
static lv_obj_t *s_btn_mel;
static int s_val[3] = { 0, 5, 0 };
static int s_focus = F_M;
static uint8_t s_icon = 0, s_melody = 9;   // 9 = Fuer Elise
static int s_edit_preset = -1;

static const int QUICK[4] = { 3, 5, 10, 15 };

static void refresh_focus() {
  for (int i = 0; i < 3; i++) {
    bool on = (s_focus == i);
    lv_obj_set_style_text_color(s_lbl[i], on ? lv_palette_main(LV_PALETTE_AMBER)
                                             : lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_lbl[i], on ? LV_OPA_20 : LV_OPA_0, 0);
  }
  lv_obj_set_style_border_width(s_btn_icon, s_focus == F_ICON ? 2 : 0, 0);
  lv_obj_set_style_border_color(s_btn_icon, lv_palette_main(LV_PALETTE_AMBER), 0);
  lv_obj_set_style_border_width(s_btn_mel, s_focus == F_MELODY ? 2 : 0, 0);
  lv_obj_set_style_border_color(s_btn_mel, lv_palette_main(LV_PALETTE_AMBER), 0);
}

static void seg_cb(lv_event_t *e) {
  s_focus = (int)(intptr_t)lv_event_get_user_data(e);
  haptic_click();
  refresh_focus();
}

static void quick_cb(lv_event_t *e) {
  int m = QUICK[(int)(intptr_t)lv_event_get_user_data(e)];
  s_val[F_H] = 0; s_val[F_M] = m; s_val[F_S] = 0;
  s_focus = F_M;
  ui_new_update();
}

static void icon_cb(lv_event_t *) { s_focus = F_ICON; ui_new_update(); }

static void mel_cb(lv_event_t *) {
  s_focus = F_MELODY;
  audio_play(&MELODIES[s_melody], false);   // beim Antippen gleich anspielen
  ui_new_update();
}

static void start_cb(lv_event_t *) {
  uint32_t total = s_val[F_H] * 3600UL + s_val[F_M] * 60UL + s_val[F_S];
  if (total == 0) { ui_toast("Zeit einstellen"); return; }

  audio_stop();
  int idx = timer_start(total, s_icon, s_melody);
  if (idx < 0) { ui_toast("Zu viele Timer"); return; }

  if (s_edit_preset >= 0) { preset_replace(s_edit_preset, total, s_icon, s_melody); s_edit_preset = -1; }
  else                     preset_remember(total, s_icon, s_melody);

  ActiveTimer *t = active_at(idx);
  if (t) ui_active_focus_id(t->id);
  haptic_bump();
  ui_goto(TILE_ACTIVE);
  ui_active_update();
  ui_toast("Timer gestartet");
}

static void cancel_cb(lv_event_t *) {
  s_edit_preset = -1;
  audio_stop();
  ui_goto(TILE_ACTIVE);
}

void ui_new_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Neuer Timer");
  lv_obj_set_style_text_font(title, &font_ui_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -142);

  s_row = lv_obj_create(p);
  lv_obj_set_size(s_row, 300, 70);
  lv_obj_align(s_row, LV_ALIGN_CENTER, 0, -92);
  lv_obj_set_style_bg_opa(s_row, LV_OPA_0, 0);
  lv_obj_set_style_border_width(s_row, 0, 0);
  lv_obj_set_style_pad_all(s_row, 0, 0);
  lv_obj_set_style_pad_column(s_row, 2, 0);
  lv_obj_set_flex_flow(s_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(s_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(s_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  for (int i = 0; i < 3; i++) {
    if (i) {
      s_sep[i - 1] = lv_label_create(s_row);
      lv_label_set_text(s_sep[i - 1], ":");
      lv_obj_set_style_text_font(s_sep[i - 1], &font_time_52, 0);
      lv_obj_set_style_text_color(s_sep[i - 1], col_dim(), 0);
    }
    s_lbl[i] = lv_label_create(s_row);
    lv_obj_set_style_text_font(s_lbl[i], &font_time_52, 0);
    lv_obj_set_style_pad_hor(s_lbl[i], 4, 0);
    lv_obj_set_style_radius(s_lbl[i], 8, 0);
    lv_obj_set_style_bg_color(s_lbl[i], lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_add_flag(s_lbl[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(s_lbl[i], 12);          // fettige Finger brauchen Platz
    lv_obj_add_event_cb(s_lbl[i], seg_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }

  lv_obj_t *chips = lv_obj_create(p);
  lv_obj_set_size(chips, 300, 44);
  lv_obj_align(chips, LV_ALIGN_CENTER, 0, -32);
  lv_obj_set_style_bg_opa(chips, LV_OPA_0, 0);
  lv_obj_set_style_border_width(chips, 0, 0);
  lv_obj_set_style_pad_all(chips, 0, 0);
  lv_obj_set_style_pad_column(chips, 4, 0);
  lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(chips, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(chips, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
  for (int i = 0; i < 4; i++) {
    char t[12]; snprintf(t, sizeof(t), "%d Min", QUICK[i]);
    make_button(chips, t, 66, 40, quick_cb, (void *)(intptr_t)i);
  }

  s_btn_mel = make_button(p, "", 146, 54, mel_cb, nullptr);
  lv_obj_align(s_btn_mel, LV_ALIGN_CENTER, -75, 32);

  s_btn_icon = make_button(p, "", 146, 54, icon_cb, nullptr);
  lv_obj_align(s_btn_icon, LV_ALIGN_CENTER, 75, 32);
  s_icon_prev = icon_create(s_btn_icon, 40);
  lv_obj_align(s_icon_prev, LV_ALIGN_LEFT_MID, 2, 0);
  lv_obj_t *il = lv_obj_get_child(s_btn_icon, 0);
  lv_obj_set_width(il, 92);
  lv_obj_align(il, LV_ALIGN_RIGHT_MID, -4, 0);

  s_btn_start = make_button(p, LV_SYMBOL_PLAY " Start", 168, 54, start_cb, nullptr);
  lv_obj_align(s_btn_start, LV_ALIGN_CENTER, -30, 104);
  button_set_color(s_btn_start, lv_palette_main(LV_PALETTE_AMBER));

  lv_obj_t *x = make_button(p, LV_SYMBOL_CLOSE, 54, 54, cancel_cb, nullptr);
  lv_obj_align(x, LV_ALIGN_CENTER, 88, 104);

  refresh_focus();
}

int ui_new_focus() { return s_focus; }

void ui_new_update() {
  lv_label_set_text_fmt(s_lbl[F_H], "%02d", s_val[F_H]);
  lv_label_set_text_fmt(s_lbl[F_M], "%02d", s_val[F_M]);
  lv_label_set_text_fmt(s_lbl[F_S], "%02d", s_val[F_S]);

  button_set_text(s_btn_icon, icon_name(s_icon));
  char mel[24]; snprintf(mel, sizeof(mel), LV_SYMBOL_AUDIO " %s", MELODIES[s_melody].name);
  button_set_text(s_btn_mel, mel);
  icon_set(s_icon_prev, s_icon, lv_color_white());
  refresh_focus();
}

void ui_new_knob(int d, int step) {
  switch (s_focus) {
    case F_H: s_val[F_H] = constrain(s_val[F_H] + d, 0, 23); break;
    case F_M: s_val[F_M] = (s_val[F_M] + d * step % 60 + 60) % 60; break;
    case F_S: s_val[F_S] = (s_val[F_S] + d * step % 60 + 60) % 60; break;
    case F_ICON:
      s_icon = (uint8_t)((s_icon + d % ICON_COUNT + ICON_COUNT) % ICON_COUNT);
      break;
    case F_MELODY:
      s_melody = (uint8_t)((s_melody + d % MELODY_COUNT + MELODY_COUNT) % MELODY_COUNT);
      audio_play(&MELODIES[s_melody], false);   // sonst waehlt man blind
      break;
  }
  ui_new_update();
}

void ui_new_load(uint32_t total_s, uint8_t icon, uint8_t melody, int edit_preset_idx) {
  s_val[F_H] = total_s / 3600;
  s_val[F_M] = (total_s / 60) % 60;
  s_val[F_S] = total_s % 60;
  s_icon = icon; s_melody = melody % MELODY_COUNT;
  s_edit_preset = edit_preset_idx;
  s_focus = F_M;
  ui_new_update();
}
