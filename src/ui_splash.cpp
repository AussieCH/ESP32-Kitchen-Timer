// Startbild: das RONDO-Logo.
//
// Bewusst gezeichnet statt als Bild eingebettet - 13 Kreise und ein Schriftzug
// kosten ein paar hundert Byte statt 260 KB, bleiben bei jeder Groesse scharf,
// und der orange Punkt kann wandern. Auf Boards mit LED-Ring laeuft er
// synchron ueber die echten LEDs: 13 Punkte, 13 LEDs, das passt genau.
#include <Arduino.h>
#include <math.h>
#include "ui.h"
#include "leds.h"

#define DOTS      13
#define RING_R    118        // Radius des Punktekreises (Vorlage: 57 % vom Halbmesser)
#define DOT_D      28
// Gesamtdauer rund 3.6 s: knapp zwei Sekunden fuer die Runde, gut eine Sekunde
// Standzeit, dann ausblenden. Schneller nimmt man das Logo nicht wahr.
#define STEP_MS   150
#define HOLD_STEPS  8        // stehenbleiben, bevor es ausblendet

#define COL_CREAM  0xFDF4E9
#define COL_ORANGE 0xEE7C25

static lv_obj_t *s_root = nullptr;
static lv_obj_t *s_dot[DOTS];
static lv_timer_t *s_timer = nullptr;
static int s_step = 0;

static void finish() {
  if (s_timer) { lv_timer_del(s_timer); s_timer = nullptr; }
  if (s_root)  { lv_obj_del(s_root); s_root = nullptr; }
  leds_off();
}

static void step_cb(lv_timer_t *) {
  if (s_step < DOTS) {
    int prev = (s_step + DOTS - 1) % DOTS;
    lv_obj_set_style_bg_color(s_dot[prev], lv_color_hex(COL_CREAM), 0);
    lv_obj_set_style_bg_color(s_dot[s_step], lv_color_hex(COL_ORANGE), 0);
    leds_single(s_step, COL_ORANGE);
  } else if (s_step == DOTS) {
    lv_obj_set_style_bg_color(s_dot[DOTS - 1], lv_color_hex(COL_CREAM), 0);
    lv_obj_set_style_bg_color(s_dot[0], lv_color_hex(COL_ORANGE), 0);  // zurueck nach oben
    leds_single(0, COL_ORANGE);
  } else if (s_step == DOTS + HOLD_STEPS) {
    lv_obj_fade_out(s_root, 400, 0);
  } else if (s_step > DOTS + HOLD_STEPS + 3) {
    finish();
    return;
  }
  s_step++;
}

static void skip_cb(lv_event_t *) { finish(); }   // antippen ueberspringt

void ui_splash_show() {
  s_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_root, SCR_W, SCR_H);
  lv_obj_center(s_root);
  lv_obj_set_style_radius(s_root, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(s_root, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(s_root, 0, 0);
  lv_obj_set_style_pad_all(s_root, 0, 0);
  lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(s_root, skip_cb, LV_EVENT_CLICKED, nullptr);

  for (int i = 0; i < DOTS; i++) {
    float a = -(float)M_PI / 2.0f + i * 2.0f * (float)M_PI / DOTS;   // oben beginnen
    s_dot[i] = lv_obj_create(s_root);
    lv_obj_set_size(s_dot[i], DOT_D, DOT_D);
    lv_obj_set_style_radius(s_dot[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_dot[i], 0, 0);
    lv_obj_set_style_bg_color(s_dot[i], lv_color_hex(i == 0 ? COL_ORANGE : COL_CREAM), 0);
    lv_obj_clear_flag(s_dot[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(s_dot[i], LV_ALIGN_CENTER,
                 (lv_coord_t)lroundf(RING_R * cosf(a)), (lv_coord_t)lroundf(RING_R * sinf(a)));
  }

  lv_obj_t *name = lv_label_create(s_root);
  lv_label_set_text(name, "RONDO");
  lv_obj_set_style_text_font(name, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(name, lv_color_hex(COL_CREAM), 0);
  lv_obj_set_style_text_letter_space(name, 6, 0);
  lv_obj_center(name);

  leds_single(0, COL_ORANGE);
  s_step = 1;
  s_timer = lv_timer_create(step_cb, STEP_MS, nullptr);
}

bool ui_splash_active() { return s_root != nullptr; }
