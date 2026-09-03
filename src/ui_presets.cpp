// Screen 3: Vorlagen ("Alle Timer").
// Tippen startet - das ist der haeufigste Fall. Editieren/Loeschen liegen hinter
// einem langen Druck, damit auf einer 250 px breiten Zeile keine drei Mini-
// Buttons nebeneinander stehen muessen.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"

static lv_obj_t *s_list, *s_hint, *s_empty;
static lv_obj_t *s_rows[MAX_PRESETS];
static int  s_row_n = 0;
static int  s_sel = 0;
static uint32_t s_sig = 0xFFFFFFFF;
static bool s_suppress_click = false;

static lv_obj_t *s_menu = nullptr;
static int s_menu_idx = -1;

static void menu_close() { if (s_menu) { lv_obj_del(s_menu); s_menu = nullptr; } }

static void start_preset(int idx) {
  Preset *p = preset_at(idx);
  if (!p) return;
  int a = timer_start(p->total_s, p->icon, p->melody);
  if (a < 0) { ui_toast("Zu viele Timer"); return; }
  ActiveTimer *t = active_at(a);
  if (t) ui_active_focus_id(t->id);
  haptic_bump();
  ui_goto(TILE_ACTIVE);
  ui_active_update();
  ui_toast("Timer gestartet");
}

static void menu_start_cb(lv_event_t *)  { int i = s_menu_idx; menu_close(); start_preset(i); }
static void menu_edit_cb(lv_event_t *)   {
  int i = s_menu_idx; menu_close();
  Preset *p = preset_at(i);
  if (!p) return;
  ui_new_load(p->total_s, p->icon, p->melody, i);
  ui_goto(TILE_NEW);
}
static void menu_del_yes(void *u) {
  preset_delete((int)(intptr_t)u);
  ui_toast("Vorlage gelöscht");
  ui_presets_update();
}
static void menu_del_cb(lv_event_t *) {
  int i = s_menu_idx; menu_close();
  ui_confirm("Vorlage löschen?", menu_del_yes, (void *)(intptr_t)i);
}
static void menu_bg_cb(lv_event_t *) { menu_close(); }

static void open_menu(int idx) {
  menu_close();
  s_menu_idx = idx;
  Preset *p = preset_at(idx);
  if (!p) return;

  s_menu = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_menu, SCR_W, SCR_H);
  lv_obj_center(s_menu);
  lv_obj_set_style_radius(s_menu, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_menu, lv_color_hex(0x101216), 0);
  lv_obj_set_style_bg_opa(s_menu, LV_OPA_90, 0);
  lv_obj_set_style_border_width(s_menu, 0, 0);
  lv_obj_clear_flag(s_menu, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_menu, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(s_menu, menu_bg_cb, LV_EVENT_CLICKED, nullptr);

  char buf[16]; fmt_time(buf, sizeof(buf), p->total_s);
  lv_obj_t *t = lv_label_create(s_menu);
  lv_label_set_text_fmt(t, "%s  %s", buf, icon_name(p->icon));
  lv_obj_set_style_text_font(t, &font_ui_20, 0);
  lv_obj_align(t, LV_ALIGN_CENTER, 0, -110);

  lv_obj_t *b;
  b = make_button(s_menu, LV_SYMBOL_PLAY " Starten", 200, 56, menu_start_cb, nullptr);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, -50);
  button_set_color(b, lv_palette_main(LV_PALETTE_AMBER));
  b = make_button(s_menu, LV_SYMBOL_EDIT " Editieren", 200, 56, menu_edit_cb, nullptr);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, 14);
  b = make_button(s_menu, LV_SYMBOL_TRASH " Löschen", 200, 56, menu_del_cb, nullptr);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, 78);
}

static void row_click_cb(lv_event_t *e) {
  if (s_suppress_click) { s_suppress_click = false; return; }
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  s_sel = idx;
  start_preset(idx);
}
static void row_long_cb(lv_event_t *e) {
  s_suppress_click = true;
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  s_sel = idx;
  haptic_bump();
  open_menu(idx);
}

static void mark_selection() {
  for (int i = 0; i < s_row_n; i++) {
    bool on = (i == s_sel);
    lv_obj_set_style_border_width(s_rows[i], on ? 2 : 0, 0);
    lv_obj_set_style_bg_opa(s_rows[i], on ? LV_OPA_30 : LV_OPA_10, 0);
  }
}

static uint32_t signature() {
  uint32_t s = preset_count() * 2654435761u;
  for (int i = 0; i < preset_count(); i++) {
    Preset *p = preset_at(i);
    s ^= (p->total_s * 31u + p->icon * 7u + p->melody) * (uint32_t)(i + 1);
  }
  return s;
}

static void rebuild() {
  lv_obj_clean(s_list);
  s_row_n = preset_count();
  for (int i = 0; i < s_row_n; i++) {
    Preset *p = preset_at(i);
    lv_color_t c = timer_color(p->icon);

    lv_obj_t *row = lv_obj_create(s_list);
    s_rows[i] = row;
    lv_obj_set_size(row, 236, 54);
    lv_obj_set_style_radius(row, 16, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x272B33), 0);
    lv_obj_set_style_border_color(row, c, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    lv_obj_add_event_cb(row, row_long_cb, LV_EVENT_LONG_PRESSED, (void *)(intptr_t)i);

    lv_obj_t *ic = icon_create(row, 40);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 2, 0);
    icon_set(ic, p->icon, c);

    char buf[16]; fmt_time(buf, sizeof(buf), p->total_s);
    lv_obj_t *tl = lv_label_create(row);
    lv_label_set_text(tl, buf);
    lv_obj_set_style_text_font(tl, &font_time_30, 0);
    lv_obj_align(tl, LV_ALIGN_LEFT_MID, 52, -8);

    lv_obj_t *nl = lv_label_create(row);
    lv_label_set_text(nl, icon_name(p->icon));
    lv_obj_set_style_text_font(nl, &font_ui_14, 0);
    lv_obj_set_style_text_color(nl, col_dim(), 0);
    lv_obj_align(nl, LV_ALIGN_LEFT_MID, 54, 16);
  }
  if (s_sel >= s_row_n) s_sel = s_row_n - 1;
  if (s_sel < 0) s_sel = 0;
  mark_selection();

  bool empty = (s_row_n == 0);
  if (empty) lv_obj_clear_flag(s_empty, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
  if (empty) lv_obj_add_flag(s_hint, LV_OBJ_FLAG_HIDDEN);   else lv_obj_clear_flag(s_hint, LV_OBJ_FLAG_HIDDEN);
}

void ui_presets_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Vorlagen");
  lv_obj_set_style_text_font(title, &font_ui_14, 0);
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
  lv_obj_set_scroll_snap_y(s_list, LV_SCROLL_SNAP_NONE);

  s_hint = lv_label_create(p);
  lv_label_set_text(s_hint, "lang tippen = mehr");
  lv_obj_set_style_text_font(s_hint, &font_ui_14, 0);
  lv_obj_set_style_text_color(s_hint, col_dim(), 0);
  lv_obj_align(s_hint, LV_ALIGN_CENTER, 0, 140);

  s_empty = lv_label_create(p);
  lv_label_set_text(s_empty, "Noch keine Vorlagen.\nEin gestarteter Timer\nwird automatisch eine.");
  lv_obj_set_style_text_align(s_empty, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(s_empty, col_dim(), 0);
  lv_obj_align(s_empty, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(s_empty, LV_OBJ_FLAG_HIDDEN);
}

void ui_presets_update() {
  uint32_t sig = signature();
  if (sig != s_sig) { s_sig = sig; rebuild(); }
}

void ui_presets_knob(int d, int) {
  if (s_row_n == 0) return;
  s_sel = constrain(s_sel + d, 0, s_row_n - 1);
  mark_selection();
  lv_obj_scroll_to_view(s_rows[s_sel], LV_ANIM_ON);
}
