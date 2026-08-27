// Screen 0: kompakte Uebersicht aller laufenden Timer.
// Eine Zeile pro Timer, Antippen holt ihn in den Aktiv-Screen. Damit muss man
// bei mehreren Timern nicht mehr durchblaettern, um zu sehen, was laeuft.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"

static lv_obj_t *s_list, *s_empty;
static lv_obj_t *s_rows[MAX_ACTIVE];
static lv_obj_t *s_time_lbl[MAX_ACTIVE];
static uint32_t s_ids[MAX_ACTIVE];
static int s_n = 0;
static int s_sel = 0;
static uint32_t s_sig = 0xFFFFFFFF;

static void row_cb(lv_event_t *e) {
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (i >= s_n) return;
  s_sel = i;
  haptic_bump();
  ui_active_focus_id(s_ids[i]);
  ui_goto(TILE_ACTIVE);
  ui_active_update();
}

static void mark_selection() {
  for (int i = 0; i < s_n; i++)
    lv_obj_set_style_border_width(s_rows[i], i == s_sel ? 2 : 0, 0);
}

static uint32_t signature() {
  uint32_t s = (uint32_t)active_count() * 2654435761u;
  for (int i = 0; i < active_count(); i++) {
    ActiveTimer *t = active_at(i);
    s ^= (t->id * 31u + t->icon * 7u + (t->paused ? 3u : 0u)
          + (t->ringing || t->expired ? 5u : 0u)) * (uint32_t)(i + 1);
  }
  return s;
}

static void rebuild() {
  lv_obj_clean(s_list);
  s_n = active_count();
  for (int i = 0; i < s_n; i++) {
    ActiveTimer *t = active_at(i);
    s_ids[i] = t->id;
    lv_color_t c = timer_color(t->color);
    bool alarm = t->ringing || t->expired;

    lv_obj_t *row = lv_obj_create(s_list);
    s_rows[i] = row;
    lv_obj_set_size(row, 236, 50);
    lv_obj_set_style_radius(row, 14, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x22262E), 0);
    lv_obj_set_style_bg_opa(row, alarm ? LV_OPA_40 : LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, c, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

    lv_obj_t *ic = icon_create(row, 40);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 2, 0);
    icon_set(ic, t->icon, c);

    s_time_lbl[i] = lv_label_create(row);
    lv_obj_set_style_text_font(s_time_lbl[i], &font_time_30, 0);
    lv_obj_set_style_text_color(s_time_lbl[i], alarm ? c : lv_color_white(), 0);
    lv_obj_align(s_time_lbl[i], LV_ALIGN_LEFT_MID, 50, 0);

    lv_obj_t *nl = lv_label_create(row);
    lv_label_set_text(nl, t->paused ? "pausiert" : (alarm ? "fertig" : icon_name(t->icon)));
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(nl, col_dim(), 0);
    lv_label_set_long_mode(nl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(nl, 92);
    lv_obj_set_style_text_align(nl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(nl, LV_ALIGN_RIGHT_MID, -4, 0);
  }
  if (s_sel >= s_n) s_sel = s_n - 1;
  if (s_sel < 0) s_sel = 0;
  mark_selection();

  if (s_n == 0) lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  else          lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
}

void ui_overview_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Laeuft gerade");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -150);

  s_list = lv_obj_create(p);
  lv_obj_set_size(s_list, 252, 220);
  lv_obj_align(s_list, LV_ALIGN_CENTER, 0, 6);
  lv_obj_set_style_bg_opa(s_list, LV_OPA_0, 0);
  lv_obj_set_style_border_width(s_list, 0, 0);
  lv_obj_set_style_pad_all(s_list, 0, 0);
  lv_obj_set_style_pad_row(s_list, 8, 0);
  lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(s_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_OFF);

  s_empty = lv_label_create(p);
  lv_label_set_text(s_empty, "Kein Timer laeuft");
  lv_obj_set_style_text_font(s_empty, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_empty, col_dim(), 0);
  lv_obj_align(s_empty, LV_ALIGN_CENTER, 0, 0);
}

void ui_overview_update() {
  uint32_t sig = signature();
  if (sig != s_sig) { s_sig = sig; rebuild(); }

  for (int i = 0; i < s_n && i < active_count(); i++) {   // Restzeiten laufen weiter
    ActiveTimer *t = active_at(i);
    char buf[16];
    fmt_time(buf, sizeof(buf), (timer_remaining_ms(t) + 999) / 1000);
    if (strcmp(lv_label_get_text(s_time_lbl[i]), buf) != 0)
      lv_label_set_text(s_time_lbl[i], buf);
  }
}

void ui_overview_knob(int d, int) {
  if (s_n == 0) return;
  s_sel = constrain(s_sel + d, 0, s_n - 1);
  mark_selection();
  lv_obj_scroll_to_view(s_rows[s_sel], LV_ANIM_ON);
}
