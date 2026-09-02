// Screen 4: Einstellungen.
// Helligkeit gibt es immer (ein Kuechentimer steht nachts im dunklen Raum),
// Lautstaerke nur auf Boards mit Lautsprecher. Zeile antippen waehlt sie aus,
// der Drehring stellt sie. Der Button fuehrt den Alarm einmal vor.
#include <Arduino.h>
#include "ui.h"
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "melodies.h"
#include "battery.h"

#define ROW_COUNT 2
#define ROW_VOL   1

static lv_obj_t *s_box[ROW_COUNT], *s_bar[ROW_COUNT], *s_val[ROW_COUNT];
static lv_obj_t *s_batt;
static int s_focus = 0;
static lv_timer_t *s_save_timer = nullptr;

static void save_cb(lv_timer_t *t) { settings_save(); lv_timer_pause(t); }

static void touch_save() {
  if (!s_save_timer) s_save_timer = lv_timer_create(save_cb, 2000, nullptr);
  lv_timer_reset(s_save_timer);
  lv_timer_resume(s_save_timer);
}

static void focus_cb(lv_event_t *e) {
  s_focus = (int)(intptr_t)lv_event_get_user_data(e);
  ui_settings_update();
}

// Alarm einmal vorfuehren: was das Board kann, macht es mit
static int s_demo_left = 0;
static lv_timer_t *s_demo_timer = nullptr;

static void demo_cb(lv_timer_t *t) {
  bool on = (s_demo_left % 2 == 1);
  haptic_buzz(on);
  leds_alarm_phase(0xFFA000, on);
  if (--s_demo_left <= 0) {
    haptic_buzz(false);
    leds_off();
    lv_timer_pause(t);
  }
}

static void test_cb(lv_event_t *) {
  audio_play(&MELODY_TEST, false);
  s_demo_left = 12;
  if (!s_demo_timer) s_demo_timer = lv_timer_create(demo_cb, 130, nullptr);
  lv_timer_reset(s_demo_timer);
  lv_timer_resume(s_demo_timer);
}

static void make_row(lv_obj_t *p, const char *name, int y, int idx) {
  lv_obj_t *box = lv_obj_create(p);
  s_box[idx] = box;
  lv_obj_set_size(box, 260, ROW_COUNT > 1 ? 76 : 84);
  lv_obj_align(box, LV_ALIGN_CENTER, 0, y);
  lv_obj_set_style_radius(box, 18, 0);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x1B1F26), 0);
  lv_obj_set_style_border_color(box, lv_palette_main(LV_PALETTE_AMBER), 0);
  lv_obj_set_style_pad_all(box, 10, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(box, focus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

  lv_obj_t *l = lv_label_create(box);
  lv_label_set_text(l, name);
  lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
  lv_obj_align(l, LV_ALIGN_TOP_LEFT, 0, 0);

  s_val[idx] = lv_label_create(box);
  lv_obj_set_style_text_font(s_val[idx], &lv_font_montserrat_16, 0);
  lv_obj_align(s_val[idx], LV_ALIGN_TOP_RIGHT, 0, 0);

  s_bar[idx] = lv_bar_create(box);
  lv_obj_set_size(s_bar[idx], 236, 16);
  lv_obj_align(s_bar[idx], LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_bar_set_range(s_bar[idx], 0, 100);
  lv_obj_set_style_bg_color(s_bar[idx], lv_color_hex(0x333944), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_bar[idx], lv_palette_main(LV_PALETTE_AMBER), LV_PART_INDICATOR);
}

void ui_settings_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Einstellungen");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -142);

  make_row(p, LV_SYMBOL_SETTINGS "  Helligkeit",  -70, 0);
  make_row(p, LV_SYMBOL_VOLUME_MAX "  Lautstaerke", 16, ROW_VOL);
  lv_obj_t *b = make_button(p, LV_SYMBOL_BELL " Alarm testen", 200, 54, test_cb, nullptr);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, 100);

  s_batt = lv_label_create(p);
  lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_batt, col_dim(), 0);
  lv_obj_align(s_batt, LV_ALIGN_CENTER, 0, 148);
}

void ui_settings_update() {
  int v[ROW_COUNT];
  v[0] = setting_brightness();
  v[ROW_VOL] = audio_get_volume();
  // Akkuzeile: auf dem Waveshare misst der Teiler das 5V-Rail, nicht die Zelle -
  // eine Prozentzahl waere dort erfunden, deshalb nur die Spannung.
  int pct = battery_percent();
  static int last_pct = -999;
  if (pct != last_pct) {
    last_pct = pct;
    if (battery_is_cell()) {
      lv_label_set_text_fmt(s_batt, LV_SYMBOL_BATTERY_FULL "  %d %%   %.2f V", pct, battery_volts());
      lv_obj_set_style_text_color(s_batt, battery_low() ? lv_palette_main(LV_PALETTE_RED)
                                                        : col_dim(), 0);
    } else {
      lv_label_set_text_fmt(s_batt, LV_SYMBOL_USB "  %.2f V", battery_volts());
      lv_obj_set_style_text_color(s_batt, col_dim(), 0);
    }
  }

  static int last[ROW_COUNT] = { -1 };
  static int last_focus = -1;
  bool same = (s_focus == last_focus);
  for (int i = 0; i < ROW_COUNT; i++) if (v[i] != last[i]) same = false;
  if (same) return;
  last_focus = s_focus;

  for (int i = 0; i < ROW_COUNT; i++) {
    last[i] = v[i];
    lv_bar_set_value(s_bar[i], v[i], LV_ANIM_OFF);
    lv_label_set_text_fmt(s_val[i], "%d %%", v[i]);
    lv_obj_set_style_border_width(s_box[i], s_focus == i ? 2 : 0, 0);
  }
}

void ui_settings_knob(int d, int step) {
  int delta = d * (step > 1 ? 5 : 2);
  if (s_focus == ROW_VOL) {
    audio_set_volume(audio_get_volume() + delta);
    if (!audio_is_playing()) audio_play(&MELODY_TEST, false);   // hoerbar statt geraten
  } else {
    setting_set_brightness(setting_brightness() + delta);
    app_apply_brightness(setting_brightness());
  }
  touch_save();
  ui_settings_update();
}
