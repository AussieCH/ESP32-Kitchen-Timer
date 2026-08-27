// Screen 4: Einstellungen.
// Seit der Ton raus ist bleibt die Helligkeit - die braucht man in einer Kueche
// nachts wirklich. Der Drehring stellt sie, der Button prueft die Haptik, die
// jetzt den Alarm traegt.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"

static lv_obj_t *s_bar, *s_val, *s_hint;
static lv_timer_t *s_save_timer = nullptr;

static void save_cb(lv_timer_t *t) { settings_save(); lv_timer_pause(t); }

static void touch_save() {
  if (!s_save_timer) s_save_timer = lv_timer_create(save_cb, 2000, nullptr);
  lv_timer_reset(s_save_timer);
  lv_timer_resume(s_save_timer);
}

// Alarmmuster einmal vorfuehren, damit man weiss, was einen erwartet
static int s_demo_left = 0;
static lv_timer_t *s_demo_timer = nullptr;

static void demo_cb(lv_timer_t *t) {
  haptic_buzz(s_demo_left % 2 == 1);
  if (--s_demo_left <= 0) { haptic_buzz(false); lv_timer_pause(t); }
}

static void test_cb(lv_event_t *) {
  if (!haptic_present()) { ui_toast("Keine Haptik gefunden"); return; }
  s_demo_left = 12;
  if (!s_demo_timer) s_demo_timer = lv_timer_create(demo_cb, 130, nullptr);
  lv_timer_reset(s_demo_timer);
  lv_timer_resume(s_demo_timer);
}

void ui_settings_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Einstellungen");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -142);

  lv_obj_t *box = lv_obj_create(p);
  lv_obj_set_size(box, 260, 84);
  lv_obj_align(box, LV_ALIGN_CENTER, 0, -40);
  lv_obj_set_style_radius(box, 18, 0);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x1B1F26), 0);
  lv_obj_set_style_border_width(box, 0, 0);
  lv_obj_set_style_pad_all(box, 12, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *l = lv_label_create(box);
  lv_label_set_text(l, LV_SYMBOL_SETTINGS "  Helligkeit");
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 0);

  s_val = lv_label_create(box);
  lv_obj_set_style_text_font(s_val, &lv_font_montserrat_16, 0);
  lv_obj_align(s_val, LV_ALIGN_TOP_RIGHT, 0, 0);

  s_bar = lv_bar_create(box);
  lv_obj_set_size(s_bar, 236, 18);
  lv_obj_align(s_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_bar_set_range(s_bar, 0, 100);
  lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x333944), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_bar, lv_palette_main(LV_PALETTE_AMBER), LV_PART_INDICATOR);

  lv_obj_t *b = make_button(p, "Alarm spueren", 200, 54, test_cb, nullptr);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, 70);

  s_hint = lv_label_create(p);
  lv_label_set_text(s_hint, "Drehring stellt die Helligkeit");
  lv_obj_set_style_text_font(s_hint, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_hint, col_dim(), 0);
  lv_obj_align(s_hint, LV_ALIGN_CENTER, 0, 132);
}

void ui_settings_update() {
  static int last = -1;
  int v = setting_brightness();
  if (v == last) return;
  last = v;
  lv_bar_set_value(s_bar, v, LV_ANIM_OFF);
  lv_label_set_text_fmt(s_val, "%d %%", v);
}

void ui_settings_knob(int d, int step) {
  setting_set_brightness(setting_brightness() + d * (step > 1 ? 5 : 2));
  app_apply_brightness(setting_brightness());
  touch_save();
  ui_settings_update();
}
