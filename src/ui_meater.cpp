// Screen: Grill-Thermometer (MEATER).
//
// Zeigt, was der Fuehler sendet, und ruft, wenn das Fleisch so weit ist: eine
// Zieltemperatur mit dem Drehring einstellen, und beim Erreichen laeuft
// derselbe Alarm wie bei einem abgelaufenen Timer.
//
// Der Fuehler meldet sich nur, wenn er aus der Ladeschale genommen wurde -
// steht er drin, bleibt es bei "suche".
#include <Arduino.h>
#include "ui.h"
#include "meater.h"
#include "alarm.h"
#include "haptics.h"
#include "gen/logo_meater.h"

#define TARGET_MIN 30
#define TARGET_MAX 99
#define TEMP_RGB   0xEE7C25        // Alarmfarbe: das Orange des Logos
#define TEMP_MELODY 19             // "Alarm", aufsteigend

static lv_obj_t *s_temp, *s_unit, *s_info, *s_state, *s_dot, *s_logo;
static lv_obj_t *s_target_box, *s_target_lbl, *s_done_lbl, *s_btn;
static bool s_fired = false;

// Garstufen in Worten - man stellt lieber "medium" ein als 58 Grad
static const char *doneness(int c) {
  if (c <= 50) return "blutig";
  if (c <= 55) return "englisch";
  if (c <= 59) return "rosa";
  if (c <= 64) return "medium";
  if (c <= 70) return "halb durch";
  if (c <= 79) return "durch";
  return "sehr durch";
}

static void arm_cb(lv_event_t *) {
  if (alarm_kind() == ALARM_TEMP) { alarm_stop(); s_fired = true; }   // Stopptaste
  else {
    setting_set_meater_armed(!setting_meater_armed());
    s_fired = false;
    settings_save();
    ui_toast(setting_meater_armed() ? "Alarm scharf" : "Alarm aus");
  }
  haptic_bump();
  ui_meater_update();
}

void ui_meater_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Grill-Thermometer");
  lv_obj_set_style_text_font(title, &font_ui_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -152);

  s_dot = lv_obj_create(p);
  lv_obj_set_size(s_dot, 10, 10);
  lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(s_dot, 0, 0);
  lv_obj_clear_flag(s_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(s_dot, LV_ALIGN_CENTER, -52, -122);

  s_state = lv_label_create(p);
  lv_obj_set_style_text_font(s_state, &font_ui_14, 0);
  lv_obj_align(s_state, LV_ALIGN_CENTER, 6, -122);

  s_temp = lv_label_create(p);
  lv_obj_set_style_text_font(s_temp, &font_time_92, 0);
  lv_obj_align(s_temp, LV_ALIGN_CENTER, -18, -56);

  s_unit = lv_label_create(p);
  lv_label_set_text(s_unit, "°C");
  lv_obj_set_style_text_font(s_unit, &font_ui_20, 0);
  lv_obj_set_style_text_color(s_unit, col_dim(), 0);

  // Ohne Verbindung stand hier "--.-" - das sieht nach Fehler aus. Stattdessen
  // das Logo des Fuehlers: dann ist klar, worauf das Geraet wartet.
  s_logo = lv_img_create(p);
  lv_img_set_src(s_logo, &logo_meater);
  lv_obj_align(s_logo, LV_ALIGN_CENTER, 0, -56);
  lv_obj_add_flag(s_logo, LV_OBJ_FLAG_HIDDEN);

  s_info = lv_label_create(p);
  lv_obj_set_style_text_font(s_info, &font_ui_14, 0);
  lv_obj_set_style_text_color(s_info, col_dim(), 0);
  lv_obj_align(s_info, LV_ALIGN_CENTER, 0, 6);

  s_target_box = lv_obj_create(p);
  lv_obj_set_size(s_target_box, 214, 42);
  lv_obj_align(s_target_box, LV_ALIGN_CENTER, 0, 50);
  lv_obj_set_style_radius(s_target_box, 14, 0);
  lv_obj_set_style_bg_color(s_target_box, lv_color_hex(0x1B1F26), 0);
  lv_obj_set_style_border_color(s_target_box, lv_color_hex(TEMP_RGB), 0);
  lv_obj_set_style_pad_all(s_target_box, 8, 0);
  lv_obj_clear_flag(s_target_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  s_target_lbl = lv_label_create(s_target_box);
  lv_obj_set_style_text_font(s_target_lbl, &font_ui_20, 0);
  lv_obj_align(s_target_lbl, LV_ALIGN_LEFT_MID, 2, 0);

  s_done_lbl = lv_label_create(s_target_box);
  lv_obj_set_style_text_font(s_done_lbl, &font_ui_14, 0);
  lv_obj_set_style_text_color(s_done_lbl, col_dim(), 0);
  lv_obj_align(s_done_lbl, LV_ALIGN_RIGHT_MID, -2, 0);

  s_btn = make_button(p, "Alarm ein", 190, 48, arm_cb, nullptr);
  lv_obj_align(s_btn, LV_ALIGN_CENTER, 0, 110);
}

void ui_meater_update() {
  MeaterState st = meater_state();
  bool live = (st == MEATER_CONNECTED) && meater_age_ms() < 30000;
  int target = setting_meater_target();
  bool armed = setting_meater_armed();
  bool ringing = (alarm_kind() == ALARM_TEMP);

  // Ausloesen: einmal pro Scharfschaltung, nicht bei jedem Messwert erneut
  if (live && armed && !s_fired && meater_tip_c() >= (float)target) {
    s_fired = true;
    alarm_start(TEMP_RGB, TEMP_MELODY, ALARM_TEMP);
    ui_goto(TILE_MEATER);
    ringing = true;
  }

  static int last_sig = -1;
  int sig = (int)st + (live ? 2 : 0) + (armed ? 4 : 0) + (ringing ? 8 : 0)
          + target * 16 + (int)(meater_tip_c() * 10) * 2048 + meater_battery() * 131072;
  if (sig == last_sig) return;
  last_sig = sig;

  // lv_label_set_text_fmt kann keine Kommazahlen - deshalb selbst formatieren
  char buf[40];
  if (live) {
    lv_obj_add_flag(s_logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_temp, LV_OBJ_FLAG_HIDDEN);
    snprintf(buf, sizeof(buf), "%.1f", meater_tip_c());
    lv_label_set_text(s_temp, buf);
    lv_obj_set_style_text_color(s_temp, ringing ? lv_color_hex(TEMP_RGB) : lv_color_white(), 0);
    lv_obj_clear_flag(s_unit, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(s_unit, s_temp, LV_ALIGN_OUT_RIGHT_MID, 4, 16);

    if (ringing) {
      lv_label_set_text(s_info, "Zieltemperatur erreicht");
      lv_obj_set_style_text_color(s_info, lv_color_hex(TEMP_RGB), 0);
    } else {
      if (meater_battery() >= 0)
        snprintf(buf, sizeof(buf), "Garraum %.0f °C   ·   Akku %d %%", meater_ambient_c(), meater_battery());
      else
        snprintf(buf, sizeof(buf), "Garraum %.0f °C", meater_ambient_c());
      lv_label_set_text(s_info, buf);
      lv_obj_set_style_text_color(s_info, col_dim(), 0);
    }
  } else {
    lv_obj_add_flag(s_temp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_logo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_unit, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_info, st == MEATER_SEARCHING ? "Fühler aus der Ladeschale nehmen" : "");
    lv_obj_set_style_text_color(s_info, col_dim(), 0);
  }

  lv_label_set_text_fmt(s_target_lbl, "Ziel  %d °C", target);
  lv_label_set_text(s_done_lbl, doneness(target));
  lv_obj_set_style_border_width(s_target_box, armed ? 2 : 0, 0);
  lv_obj_set_style_text_color(s_target_lbl, armed ? lv_color_hex(TEMP_RGB) : lv_color_white(), 0);

  if (ringing) {
    button_set_text(s_btn, LV_SYMBOL_STOP " Stopp");
    button_set_color(s_btn, lv_color_hex(TEMP_RGB));
  } else {
    button_set_text(s_btn, armed ? "Alarm aus" : "Alarm ein");
    lv_obj_set_style_bg_color(s_btn, lv_color_hex(0x272B33), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(s_btn, 0), lv_color_white(), 0);
  }

  const char *txt = live ? meater_name() : (st == MEATER_SEARCHING ? "suche ..." : "aus");
  lv_color_t col = live ? lv_palette_main(LV_PALETTE_GREEN)
                        : (st == MEATER_SEARCHING ? lv_palette_main(LV_PALETTE_AMBER) : col_dim());
  lv_label_set_text(s_state, txt);
  lv_obj_set_style_text_color(s_state, col, 0);
  lv_obj_set_style_bg_color(s_dot, col, 0);
}

// Der Drehring hat auf diesem Screen nur eine Aufgabe: die Zieltemperatur.
void ui_meater_knob(int d, int step) {
  int t = setting_meater_target() + d * (step > 1 ? 5 : 1);
  setting_set_meater_target(constrain(t, TARGET_MIN, TARGET_MAX));
  s_fired = false;                     // neues Ziel -> darf wieder ausloesen
  settings_save();
  ui_meater_update();
}
