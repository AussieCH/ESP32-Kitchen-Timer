// Rahmen der Bedienung.
//
// Eine Regel, ueberall gleich:
//   horizontal wischen = Screen wechseln
//   Drehring          = immer der Kontext im aktuellen Screen, nie Navigation
//   vertikal wischen  = innerhalb einer Liste blaettern
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"

static lv_obj_t *s_tv;
static lv_obj_t *s_tiles[TILE_COUNT];
static lv_obj_t *s_dots[TILE_COUNT];
static int  s_tile = TILE_ACTIVE;

static lv_obj_t *s_debug = nullptr;
static lv_obj_t *s_toast = nullptr;
static lv_timer_t *s_toast_timer = nullptr;

static lv_obj_t *s_confirm = nullptr;
static ui_yes_cb_t s_confirm_cb = nullptr;
static void *s_confirm_user = nullptr;

lv_color_t timer_color(uint8_t idx) {
  static const lv_palette_t pal[8] = {
    LV_PALETTE_AMBER, LV_PALETTE_LIGHT_BLUE, LV_PALETTE_GREEN, LV_PALETTE_PINK,
    LV_PALETTE_PURPLE, LV_PALETTE_ORANGE, LV_PALETTE_CYAN,  LV_PALETTE_LIME };
  return lv_palette_main(pal[idx % 8]);
}
lv_color_t col_dim() { return lv_color_hex(0x8A909B); }

void fmt_time(char *buf, size_t n, uint32_t secs) {
  if (secs >= 3600) snprintf(buf, n, "%u:%02u:%02u", secs / 3600, (secs / 60) % 60, secs % 60);
  else              snprintf(buf, n, "%02u:%02u", secs / 60, secs % 60);
}

// ---------------------------------------------------------------- Buttons
static void press_cb(lv_event_t *) { haptic_click(); }

lv_obj_t *make_button(lv_obj_t *parent, const char *txt, lv_coord_t w, lv_coord_t h,
                      lv_event_cb_t cb, void *user) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_radius(b, h / 2, 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x272B33), 0);
  lv_obj_set_style_bg_color(b, lv_color_hex(0x515866), LV_STATE_PRESSED);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_border_width(b, 0, 0);
  lv_obj_set_style_pad_all(b, 4, 0);

  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
  lv_obj_set_width(l, w - 12);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(l);

  lv_obj_add_event_cb(b, press_cb, LV_EVENT_PRESSED, nullptr);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
  return b;
}

void button_set_text(lv_obj_t *btn, const char *txt) {
  if (btn) lv_label_set_text(lv_obj_get_child(btn, 0), txt);
}
void button_set_color(lv_obj_t *btn, lv_color_t c) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, c, 0);
  lv_obj_set_style_text_color(lv_obj_get_child(btn, 0), lv_color_black(), 0);
}

// ---------------------------------------------------------------- Toast
static void toast_hide_cb(lv_timer_t *t) {
  lv_obj_add_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
  lv_timer_pause(t);
}

void ui_toast(const char *txt) {
  if (!s_toast) {
    s_toast = lv_label_create(lv_layer_top());
    lv_obj_set_style_bg_color(s_toast, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_toast, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(s_toast, 10, 0);
    lv_obj_set_style_radius(s_toast, 14, 0);
    lv_obj_set_style_text_color(s_toast, lv_color_white(), 0);
    lv_obj_align(s_toast, LV_ALIGN_CENTER, 0, 118);
    s_toast_timer = lv_timer_create(toast_hide_cb, 1400, nullptr);
  }
  lv_label_set_text(s_toast, txt);
  lv_obj_align(s_toast, LV_ALIGN_CENTER, 0, 118);
  lv_obj_clear_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_toast);
  lv_timer_reset(s_toast_timer);
  lv_timer_resume(s_toast_timer);
}

// ---------------------------------------------------------------- Rueckfrage
static void confirm_close() {
  if (s_confirm) { lv_obj_del(s_confirm); s_confirm = nullptr; }
}
static void confirm_yes_cb(lv_event_t *) {
  ui_yes_cb_t cb = s_confirm_cb; void *u = s_confirm_user;
  confirm_close();
  if (cb) cb(u);
}
static void confirm_no_cb(lv_event_t *) { confirm_close(); }

void ui_confirm(const char *question, ui_yes_cb_t yes_cb, void *user) {
  confirm_close();
  s_confirm_cb = yes_cb; s_confirm_user = user;

  s_confirm = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_confirm, SCR_W, SCR_H);
  lv_obj_center(s_confirm);
  lv_obj_set_style_radius(s_confirm, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_confirm, lv_color_hex(0x101216), 0);
  lv_obj_set_style_bg_opa(s_confirm, LV_OPA_90, 0);
  lv_obj_set_style_border_width(s_confirm, 0, 0);
  lv_obj_clear_flag(s_confirm, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *l = lv_label_create(s_confirm);
  lv_label_set_text(l, question);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(l, 240);
  lv_obj_align(l, LV_ALIGN_CENTER, 0, -40);

  lv_obj_t *yes = make_button(s_confirm, "Ja", 110, 56, confirm_yes_cb, nullptr);
  lv_obj_align(yes, LV_ALIGN_CENTER, -60, 60);
  button_set_color(yes, lv_palette_main(LV_PALETTE_RED));

  lv_obj_t *no = make_button(s_confirm, "Nein", 110, 56, confirm_no_cb, nullptr);
  lv_obj_align(no, LV_ALIGN_CENTER, 60, 60);
}


// ---------------------------------------------------------------- Rahmen
static void dots_update() {
  for (int i = 0; i < TILE_COUNT; i++) {
    bool on = (i == s_tile);
    lv_obj_set_style_bg_color(s_dots[i], on ? lv_color_white() : lv_color_hex(0x555B66), 0);
    lv_obj_set_size(s_dots[i], on ? 8 : 6, on ? 8 : 6);
  }
}

static void tile_changed_cb(lv_event_t *) {
  lv_obj_t *act = lv_tileview_get_tile_act(s_tv);
  for (int i = 0; i < TILE_COUNT; i++) if (s_tiles[i] == act) s_tile = i;
  dots_update();
  switch (s_tile) {
    case TILE_OVERVIEW: ui_overview_update(); break;
    case TILE_ACTIVE:   ui_active_update();   break;
    case TILE_NEW:      ui_new_update();      break;
    case TILE_PRESETS:  ui_presets_update();  break;
    case TILE_SETTINGS: ui_settings_update(); break;
  }
}

static void gesture_cb(lv_event_t *) {
  if (s_confirm) return;
  lv_dir_t d = lv_indev_get_gesture_dir(lv_indev_get_act());
  if ((d == LV_DIR_TOP || d == LV_DIR_BOTTOM) && s_tile == TILE_ACTIVE) ui_active_gesture(d);
}

void ui_init() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_add_event_cb(scr, gesture_cb, LV_EVENT_GESTURE, nullptr);

  s_tv = lv_tileview_create(scr);
  lv_obj_set_size(s_tv, SCR_W, SCR_H);
  lv_obj_set_style_bg_color(s_tv, lv_color_black(), 0);
  lv_obj_set_scrollbar_mode(s_tv, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(s_tv, tile_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);

  for (int i = 0; i < TILE_COUNT; i++) {
    s_tiles[i] = lv_tileview_add_tile(s_tv, i, 0, LV_DIR_HOR);
    lv_obj_set_style_pad_all(s_tiles[i], 0, 0);
    lv_obj_clear_flag(s_tiles[i], LV_OBJ_FLAG_SCROLLABLE);
  }

  ui_overview_create(s_tiles[TILE_OVERVIEW]);
  ui_active_create(s_tiles[TILE_ACTIVE]);
  ui_new_create(s_tiles[TILE_NEW]);
  ui_presets_create(s_tiles[TILE_PRESETS]);
  ui_settings_create(s_tiles[TILE_SETTINGS]);

  lv_obj_t *dotbox = lv_obj_create(lv_layer_top());
  lv_obj_set_size(dotbox, 100, 16);
  lv_obj_align(dotbox, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_style_bg_opa(dotbox, LV_OPA_0, 0);
  lv_obj_set_style_border_width(dotbox, 0, 0);
  lv_obj_set_style_pad_all(dotbox, 0, 0);
  lv_obj_set_flex_flow(dotbox, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(dotbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(dotbox, 7, 0);
  lv_obj_clear_flag(dotbox, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < TILE_COUNT; i++) {
    s_dots[i] = lv_obj_create(dotbox);
    lv_obj_set_size(s_dots[i], 6, 6);
    lv_obj_set_style_radius(s_dots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_dots[i], 0, 0);
    lv_obj_clear_flag(s_dots[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  }
  dots_update();
  lv_obj_set_tile_id(s_tv, TILE_ACTIVE, 0, LV_ANIM_OFF);   // Start auf dem Aktiv-Screen
  ui_active_update();
}

void ui_debug_set(const char *txt) {
  if (!s_debug) {
    s_debug = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_font(s_debug, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_debug, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_bg_color(s_debug, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_debug, LV_OPA_70, 0);
    lv_obj_set_style_pad_all(s_debug, 3, 0);
    lv_obj_clear_flag(s_debug, LV_OBJ_FLAG_CLICKABLE);
  }
  lv_label_set_text(s_debug, txt);
  lv_obj_align(s_debug, LV_ALIGN_CENTER, 0, 70);
  lv_obj_move_foreground(s_debug);
}

int ui_current_tile() { return s_tile; }

void ui_goto(int tile) {
  s_tile = tile;
  lv_obj_set_tile_id(s_tv, tile, 0, LV_ANIM_ON);
  dots_update();
}

void ui_tick() {
  switch (s_tile) {
    case TILE_OVERVIEW: ui_overview_update(); break;
    case TILE_ACTIVE:   ui_active_update();   break;
    case TILE_NEW:      ui_new_update();      break;
    case TILE_PRESETS:  ui_presets_update();  break;
    case TILE_SETTINGS: ui_settings_update(); break;
  }
}

void ui_on_knob(int delta, int step) {
  if (s_confirm) return;
  haptic_click();
  switch (s_tile) {
    case TILE_OVERVIEW: ui_overview_knob(delta, step);  break;
    case TILE_ACTIVE:   ui_active_knob(delta, step);   break;
    case TILE_NEW:      ui_new_knob(delta, step);      break;
    case TILE_PRESETS:  ui_presets_knob(delta, step);  break;
    case TILE_SETTINGS: ui_settings_knob(delta, step); break;
  }
}
