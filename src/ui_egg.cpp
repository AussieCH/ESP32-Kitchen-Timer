// Screen: Eieruhr.
// Rechnet die Kochzeit aus Groesse, Starttemperatur und gewuenschter
// Konsistenz - statt sie zu raten oder eine Tabelle im Kopf zu haben.
//
// Formel nach Charles D. N. Williams (Exeter): die Zeit, bis das Dotterzentrum
// die Zieltemperatur erreicht.
//   t = 0.451 * M^(2/3) * ln( 0.76 * (T_ei - T_wasser) / (T_dotter - T_wasser) )
// M in Gramm, Temperaturen in Grad Celsius. Die Konstanten unten sind auf
// uebliche Ergebnisse abgeglichen (M-Ei aus dem Kuehlschrank: weich ~4:35,
// wachsweich ~6:00, hart ~8:45).
#include <Arduino.h>
#include <math.h>
#include "ui.h"
#include "haptics.h"

#define EGG_ICON   1        // Icon "Ei"
#define EGG_MELODY 9

#define ROWS 3
static const char *LBL[ROWS]  = { "Grösse", "Start", "Ergebnis" };
// Schweizer Verkaufsgroessen - so steht es auf der Eierschachtel, danach waehlt
// man im Laden. Gerechnet wird mit der Mitte des jeweiligen Bereichs (MASS).
static const char *SIZE_T[4]  = { "S  < 53 g", "M  53-63 g", "L  63-73 g", "XL  > 73 g" };
static const char *START_T[2] = { "Kühlschrank", "Zimmer" };
static const char *DONE_T[3]  = { "weich", "wachsweich", "hart" };
static const int   COUNT[ROWS] = { 4, 2, 3 };

static const float MASS[4]  = { 48.0f, 58.0f, 68.0f, 78.0f };   // Mitten der Klassen
static const float T_EGG[2] = { 4.0f, 20.0f };
static const float T_YOLK[3] = { 63.0f, 70.0f, 80.0f };

static lv_obj_t *s_row[ROWS], *s_val[ROWS], *s_time, *s_btn;
static int s_sel[ROWS] = { 1, 0, 0 };
static int s_focus = 2;

static uint32_t egg_seconds() {
  const float TW = 100.0f;
  float m = powf(MASS[s_sel[0]], 2.0f / 3.0f);
  float arg = 0.76f * (T_EGG[s_sel[1]] - TW) / (T_YOLK[s_sel[2]] - TW);
  float minutes = 0.451f * m * logf(arg);
  if (minutes < 0.5f) minutes = 0.5f;
  return (uint32_t)(minutes * 60.0f + 0.5f);
}

static void row_cb(lv_event_t *e) {
  s_focus = (int)(intptr_t)lv_event_get_user_data(e);
  haptic_click();
  ui_egg_update();
}

static void start_cb(lv_event_t *) {
  int idx = timer_start(egg_seconds(), EGG_ICON, EGG_MELODY);
  if (idx < 0) { ui_toast("Zu viele Timer"); return; }
  ActiveTimer *t = active_at(idx);
  if (t) ui_active_focus_id(t->id);
  haptic_bump();
  ui_goto(TILE_ACTIVE);
  ui_active_update();
  ui_toast("Eieruhr läuft");
}

void ui_egg_create(lv_obj_t *p) {
  lv_obj_t *title = lv_label_create(p);
  lv_label_set_text(title, "Eieruhr");
  lv_obj_set_style_text_font(title, &font_ui_14, 0);
  lv_obj_set_style_text_color(title, col_dim(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -152);

  for (int i = 0; i < ROWS; i++) {
    lv_obj_t *row = lv_obj_create(p);
    s_row[i] = row;
    lv_obj_set_size(row, 230, 40);
    lv_obj_align(row, LV_ALIGN_CENTER, 0, -114 + i * 46);
    lv_obj_set_style_radius(row, 14, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x1B1F26), 0);
    lv_obj_set_style_border_color(row, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

    lv_obj_t *l = lv_label_create(row);
    lv_label_set_text(l, LBL[i]);
    lv_obj_set_style_text_font(l, &font_ui_14, 0);
    lv_obj_set_style_text_color(l, col_dim(), 0);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);

    s_val[i] = lv_label_create(row);
    lv_obj_set_style_text_font(s_val[i], &font_ui_16, 0);
    lv_obj_align(s_val[i], LV_ALIGN_RIGHT_MID, 0, 0);
  }

  s_time = lv_label_create(p);
  lv_obj_set_style_text_font(s_time, &font_time_52, 0);
  lv_obj_align(s_time, LV_ALIGN_CENTER, 0, 42);

  s_btn = make_button(p, LV_SYMBOL_PLAY " Kochen", 200, 54, start_cb, nullptr);
  lv_obj_align(s_btn, LV_ALIGN_CENTER, 0, 108);
  button_set_color(s_btn, lv_palette_main(LV_PALETTE_AMBER));
}

void ui_egg_update() {
  const char *const *tab[ROWS] = { SIZE_T, START_T, DONE_T };
  for (int i = 0; i < ROWS; i++) {
    lv_label_set_text(s_val[i], tab[i][s_sel[i]]);
    lv_obj_set_style_border_width(s_row[i], s_focus == i ? 2 : 0, 0);
  }
  char buf[16];
  fmt_time(buf, sizeof(buf), egg_seconds());
  lv_label_set_text(s_time, buf);
}

void ui_egg_knob(int d, int) {
  int n = COUNT[s_focus];
  s_sel[s_focus] = (s_sel[s_focus] + d % n + n) % n;
  ui_egg_update();
}
