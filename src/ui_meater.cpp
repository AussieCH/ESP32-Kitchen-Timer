// Screen: Grill-Thermometer (MEATER).
//
// Zeigt an, was der Fuehler sendet: Kerntemperatur gross, darunter Garraum,
// Akku und Verbindungszustand. Der Fuehler meldet sich nur, wenn er aus der
// Ladeschale genommen wurde - steht er drin, bleibt es bei "suche".
#include <Arduino.h>
#include "ui.h"
#include "meater.h"

static lv_obj_t *s_temp, *s_unit, *s_amb, *s_batt, *s_state, *s_dot;

void ui_meater_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Grill-Thermometer");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -150);

  s_temp = lv_label_create(p);
  lv_obj_set_style_text_font(s_temp, &font_time_92, 0);
  lv_obj_align(s_temp, LV_ALIGN_CENTER, -18, -46);

  s_unit = lv_label_create(p);
  lv_label_set_text(s_unit, "°C");
  lv_obj_set_style_text_font(s_unit, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_unit, col_dim(), 0);
  // Ausrichtung passiert in ui_meater_update: hier ist die Zahl noch leer,
  // und das Label waechst erst mit dem Text.

  s_amb = lv_label_create(p);
  lv_obj_set_style_text_font(s_amb, &lv_font_montserrat_16, 0);
  lv_obj_align(s_amb, LV_ALIGN_CENTER, 0, 22);

  s_batt = lv_label_create(p);
  lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_batt, col_dim(), 0);
  lv_obj_align(s_batt, LV_ALIGN_CENTER, 0, 52);

  // Verbindungszustand: Punkt plus Wort, damit man es auch ohne Lesen erfasst
  s_dot = lv_obj_create(p);
  lv_obj_set_size(s_dot, 12, 12);
  lv_obj_set_style_radius(s_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(s_dot, 0, 0);
  lv_obj_clear_flag(s_dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(s_dot, LV_ALIGN_CENTER, -58, 108);

  s_state = lv_label_create(p);
  lv_obj_set_style_text_font(s_state, &lv_font_montserrat_16, 0);
  lv_obj_align(s_state, LV_ALIGN_CENTER, 8, 108);
}

void ui_meater_update() {
  MeaterState st = meater_state();
  bool live = (st == MEATER_CONNECTED) && meater_age_ms() < 30000;

  static int last_sig = -1;
  int sig = (int)st * 100000 + (int)(meater_tip_c() * 10) * 10 + (live ? 1 : 0)
          + meater_battery() * 1000;
  if (sig == last_sig) return;
  last_sig = sig;

  if (live) {
    // Achtung: lv_label_set_text_fmt kann keine Kommazahlen (LVGLs eigenes
    // snprintf kennt %f nicht) - deshalb hier immer erst selbst formatieren.
    char buf[24];
    snprintf(buf, sizeof(buf), "%.1f", meater_tip_c());
    lv_label_set_text(s_temp, buf);
    lv_obj_set_style_text_color(s_temp, lv_color_white(), 0);
    lv_obj_clear_flag(s_unit, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(s_unit, s_temp, LV_ALIGN_OUT_RIGHT_MID, 4, 16);
    snprintf(buf, sizeof(buf), "Garraum  %.0f °C", meater_ambient_c());
    lv_label_set_text(s_amb, buf);
    lv_obj_clear_flag(s_amb, LV_OBJ_FLAG_HIDDEN);
    if (meater_battery() >= 0) lv_label_set_text_fmt(s_batt, "Fuehler-Akku  %d %%", meater_battery());
    else                       lv_label_set_text(s_batt, "");
  } else {
    lv_label_set_text(s_temp, "--.-");
    lv_obj_set_style_text_color(s_temp, col_dim(), 0);
    lv_obj_add_flag(s_unit, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_amb, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s_batt, st == MEATER_SEARCHING ? "Fuehler aus der Ladeschale nehmen" : "");
  }

  const char *txt = live ? meater_name() : (st == MEATER_SEARCHING ? "suche ..." : "aus");
  lv_color_t col = live ? lv_palette_main(LV_PALETTE_GREEN)
                        : (st == MEATER_SEARCHING ? lv_palette_main(LV_PALETTE_AMBER) : col_dim());
  lv_label_set_text(s_state, txt);
  lv_obj_set_style_text_color(s_state, col, 0);
  lv_obj_set_style_bg_color(s_dot, col, 0);
}
