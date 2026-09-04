// Screen: Einstellungen.
//
// Drei Zeilen: Helligkeit und Lautstärke als Balken, Sprache als Auswahl.
// Zeile antippen wählt sie aus, der Drehring stellt sie. Der Knopf führt den
// Alarm einmal vor, ganz unten steht der Akkustand.
#include <Arduino.h>
#include "ui.h"
#include "lang.h"
#include "haptics.h"
#include "audio.h"
#include "leds.h"
#include "melodies.h"
#include "battery.h"

#define ROW_BRIGHT 0
#define ROW_VOL    1
#define ROW_LANG   2
#define ROW_COUNT  3

static lv_obj_t *s_box[ROW_COUNT], *s_bar[ROW_COUNT], *s_val[ROW_COUNT], *s_lbl[ROW_COUNT];
static lv_obj_t *s_batt, *s_title, *s_btn;
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
  if (--s_demo_left <= 0) { haptic_buzz(false); leds_off(); lv_timer_pause(t); }
}

static void test_cb(lv_event_t *) {
  audio_play(&MELODY_TEST, false);
  s_demo_left = 12;
  if (!s_demo_timer) s_demo_timer = lv_timer_create(demo_cb, 130, nullptr);
  lv_timer_reset(s_demo_timer);
  lv_timer_resume(s_demo_timer);
}

// Balkenzeilen sind hoeher als die Auswahlzeile - auf dem runden Panel zaehlt
// jeder Millimeter, und ein Balken fuer "Deutsch/English" waere Unsinn.
static void make_row(lv_obj_t *p, int idx, int y, int h, bool with_bar) {
  lv_obj_t *box = lv_obj_create(p);
  s_box[idx] = box;
  lv_obj_set_size(box, 250, h);
  lv_obj_align(box, LV_ALIGN_CENTER, 0, y);
  lv_obj_set_style_radius(box, 16, 0);
  lv_obj_set_style_bg_color(box, lv_color_hex(0x1B1F26), 0);
  lv_obj_set_style_border_color(box, lv_palette_main(LV_PALETTE_AMBER), 0);
  lv_obj_set_style_pad_all(box, 8, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(box, focus_cb, LV_EVENT_CLICKED, (void *)(intptr_t)idx);

  s_lbl[idx] = lv_label_create(box);
  lv_obj_set_style_text_font(s_lbl[idx], &font_ui_16, 0);
  lv_obj_align(s_lbl[idx], with_bar ? LV_ALIGN_TOP_LEFT : LV_ALIGN_LEFT_MID, 0, 0);

  s_val[idx] = lv_label_create(box);
  lv_obj_set_style_text_font(s_val[idx], &font_ui_16, 0);
  lv_obj_align(s_val[idx], with_bar ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_RIGHT_MID, 0, 0);

  if (!with_bar) { s_bar[idx] = nullptr; return; }
  s_bar[idx] = lv_bar_create(box);
  lv_obj_set_size(s_bar[idx], 226, 14);
  lv_obj_align(s_bar[idx], LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_bar_set_range(s_bar[idx], 0, 100);
  lv_obj_set_style_bg_color(s_bar[idx], lv_color_hex(0x333944), LV_PART_MAIN);
  lv_obj_set_style_bg_color(s_bar[idx], lv_palette_main(LV_PALETTE_AMBER), LV_PART_INDICATOR);
}

void ui_settings_create(lv_obj_t *p) {
  s_title = lv_label_create(p);
  lv_obj_set_style_text_font(s_title, &font_ui_14, 0);
  lv_obj_set_style_text_color(s_title, col_dim(), 0);
  lv_obj_align(s_title, LV_ALIGN_CENTER, 0, -148);

  make_row(p, ROW_BRIGHT, -88, 64, true);
  make_row(p, ROW_VOL,    -16, 64, true);
  make_row(p, ROW_LANG,    48, 42, false);

  s_btn = make_button(p, "", 190, 46, test_cb, nullptr);
  lv_obj_align(s_btn, LV_ALIGN_CENTER, 0, 104);

  s_batt = lv_label_create(p);
  lv_obj_set_style_text_font(s_batt, &font_ui_14, 0);
  lv_obj_set_style_text_color(s_batt, col_dim(), 0);
  lv_obj_align(s_batt, LV_ALIGN_CENTER, 0, 148);
}

void ui_settings_update() {
  int v[ROW_COUNT] = { setting_brightness(), audio_get_volume(), setting_lang() };

  // Akkuzeile: auf Boards, die am Strom haengen, waere eine Prozentzahl erfunden
  int pct = battery_percent();
  static int last_pct = -999;
  static uint8_t last_rev = 255;
  if (pct != last_pct || last_rev != lang_rev()) {
    last_pct = pct;
    char bbuf[48];
    if (battery_is_cell()) {
      // lv_label_set_text_fmt kann keine Kommazahlen - selbst formatieren
      snprintf(bbuf, sizeof(bbuf), LV_SYMBOL_BATTERY_FULL "  %d %%   %.2f V", pct, battery_volts());
      lv_label_set_text(s_batt, bbuf);
      lv_obj_set_style_text_color(s_batt, battery_low() ? lv_palette_main(LV_PALETTE_RED)
                                                        : col_dim(), 0);
    } else {
      snprintf(bbuf, sizeof(bbuf), LV_SYMBOL_USB "  %.2f V", battery_volts());
      lv_label_set_text(s_batt, bbuf);
      lv_obj_set_style_text_color(s_batt, col_dim(), 0);
    }
  }

  static int last[ROW_COUNT] = { -1, -1, -1 };
  static int last_focus = -1;
  bool same = (s_focus == last_focus && last_rev == lang_rev());
  for (int i = 0; i < ROW_COUNT; i++) if (v[i] != last[i]) same = false;
  if (same) return;
  last_focus = s_focus;
  last_rev = lang_rev();

  lv_label_set_text(s_title, T(T_SETTINGS));
  button_set_text(s_btn, lang_btn(LV_SYMBOL_BELL, T_TEST_ALARM));

  static const TxtId NAMES[ROW_COUNT] = { T_BRIGHTNESS, T_VOLUME, T_LANGUAGE };
  static const char *SYMS[ROW_COUNT] = { LV_SYMBOL_SETTINGS, LV_SYMBOL_VOLUME_MAX, LV_SYMBOL_LIST };
  for (int i = 0; i < ROW_COUNT; i++) {
    last[i] = v[i];
    char lbl[48];
    snprintf(lbl, sizeof(lbl), "%s  %s", SYMS[i], T(NAMES[i]));
    lv_label_set_text(s_lbl[i], lbl);
    if (s_bar[i]) {
      lv_bar_set_value(s_bar[i], v[i], LV_ANIM_OFF);
      lv_label_set_text_fmt(s_val[i], "%d %%", v[i]);
    } else {
      lv_label_set_text(s_val[i], lang_label((Lang)v[i]));
    }
    lv_obj_set_style_border_width(s_box[i], s_focus == i ? 2 : 0, 0);
  }
}

void ui_settings_knob(int d, int step) {
  if (s_focus == ROW_LANG) {
    // Sprache umschalten - der Rest der Oberflaeche zeichnet sich neu, weil
    // jede Zeichen-Wache die Sprachnummer mitfuehrt.
    setting_set_lang((setting_lang() + (d > 0 ? 1 : LANG_COUNT - 1)) % LANG_COUNT);
  } else {
    int delta = d * (step > 1 ? 5 : 2);
    if (s_focus == ROW_VOL) {
      audio_set_volume(audio_get_volume() + delta);
      if (!audio_is_playing()) audio_play(&MELODY_TEST, false);   // hoerbar statt geraten
    } else {
      setting_set_brightness(setting_brightness() + delta);
      app_apply_brightness(setting_brightness());
    }
  }
  touch_save();
  ui_settings_update();
}
