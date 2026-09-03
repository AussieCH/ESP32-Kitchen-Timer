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
static lv_obj_t *s_mark = nullptr;
static lv_timer_t *s_timer = nullptr;
static int s_step = 0;

// ---------------------------------------------------------------- Logo
// Als eigener Baustein, weil es zweimal gebraucht wird: beim Start und als
// Ruhebild, wenn kein Timer laeuft.
lv_obj_t *rondo_mark_create(lv_obj_t *parent, int ring_r, int dot_d, bool with_word) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_size(box, 2 * ring_r + dot_d + 4, 2 * ring_r + dot_d + 4);
  lv_obj_set_style_bg_opa(box, LV_OPA_0, 0);
  lv_obj_set_style_border_width(box, 0, 0);
  lv_obj_set_style_pad_all(box, 0, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  for (int i = 0; i < DOTS; i++) {
    float a = -(float)M_PI / 2.0f + i * 2.0f * (float)M_PI / DOTS;   // oben beginnen
    lv_obj_t *d = lv_obj_create(box);
    lv_obj_set_size(d, dot_d, dot_d);
    lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_set_style_bg_color(d, lv_color_hex(COL_CREAM), 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(d, LV_ALIGN_CENTER,
                 (lv_coord_t)lroundf(ring_r * cosf(a)), (lv_coord_t)lroundf(ring_r * sinf(a)));
  }
  if (with_word) {
    lv_obj_t *name = lv_label_create(box);
    lv_label_set_text(name, "RONDO");
    lv_obj_set_style_text_font(name, &font_ui_28, 0);
    lv_obj_set_style_text_color(name, lv_color_hex(COL_CREAM), 0);
    lv_obj_set_style_text_letter_space(name, 6, 0);
    lv_obj_center(name);
  }
  return box;
}

void rondo_mark_highlight(lv_obj_t *mark, int idx) {
  for (int i = 0; i < DOTS; i++)
    lv_obj_set_style_bg_color(lv_obj_get_child(mark, i),
                              lv_color_hex(i == idx ? COL_ORANGE : COL_CREAM), 0);
}

static void finish() {
  if (s_timer) { lv_timer_del(s_timer); s_timer = nullptr; }
  if (s_root)  { lv_obj_del(s_root); s_root = nullptr; }
  leds_off();
}

static void step_cb(lv_timer_t *) {
  if (s_step < DOTS) {
    rondo_mark_highlight(s_mark, s_step);
    leds_single(s_step, COL_ORANGE);
  } else if (s_step == DOTS) {
    rondo_mark_highlight(s_mark, 0);                                   // zurueck nach oben
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

  s_mark = rondo_mark_create(s_root, RING_R, DOT_D, true);
  lv_obj_center(s_mark);
  rondo_mark_highlight(s_mark, 0);

  leds_single(0, COL_ORANGE);
  s_step = 1;
  s_timer = lv_timer_create(step_cb, STEP_MS, nullptr);
}

bool ui_splash_active() { return s_root != nullptr; }
