// Host-Simulator: rendert die echten UI-Dateien in einen Framebuffer, schreibt
// Screenshots und prueft, ob Widgets aus dem runden Sichtfeld ragen.
#include <Arduino.h>
#include <unistd.h>
#include <lvgl.h>
#include "ui.h"
#include "timers.h"
#include "meater.h"

static lv_color_t fb[SCR_W * SCR_H];
static int violations = 0;

static void flush_cb(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *px) {
  int w = a->x2 - a->x1 + 1;
  for (int y = a->y1; y <= a->y2; y++)
    for (int x = a->x1; x <= a->x2; x++)
      fb[y * SCR_W + x] = px[(y - a->y1) * w + (x - a->x1)];
  lv_disp_flush_ready(d);
}

static void settle(int ms) {
  for (int i = 0; i < ms / 10; i++) { lv_timer_handler(); usleep(10000); }
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(nullptr);
}

static void shot(const char *name) {
  char path[256];
  snprintf(path, sizeof(path), "tools/sim/out/%s.ppm", name);
  FILE *f = fopen(path, "wb");
  fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
  for (int i = 0; i < SCR_W * SCR_H; i++) {
    int x = i % SCR_W - 180, y = i / SCR_W - 180;
    uint32_t c = (x * x + y * y > 180 * 180) ? 0x00202020 : lv_color_to32(fb[i]);
    uint8_t rgb[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

// Alles, was ausserhalb des Kreises liegt, ist auf diesem Panel unsichtbar.
// Geprueft wird die tatsaechlich SICHTBARE Flaeche: LVGL beschneidet Kinder am
// Elternrand, ein Listeneintrag ragt also nur scheinbar nach unten heraus.
static void check(lv_obj_t *o, const char *scenario, lv_area_t clip) {
  if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) return;
  lv_area_t own; lv_obj_get_coords(o, &own);
  lv_area_t a;
  if (!_lv_area_intersect(&a, &own, &clip)) return;      // vollstaendig abgeschnitten
  int w = a.x2 - a.x1 + 1, h = a.y2 - a.y1 + 1;
  if (a.x2 < 0 || a.x1 > SCR_W - 1 || a.y2 < 0 || a.y1 > SCR_H - 1) return;
  bool paints = lv_obj_get_style_bg_opa(o, LV_PART_MAIN) != LV_OPA_0
             || lv_obj_get_style_border_width(o, LV_PART_MAIN) != 0
             || lv_obj_check_type(o, &lv_label_class) || lv_obj_check_type(o, &lv_img_class);
  if (paints && (w < 300 || h < 300)) {                       // Vollflaechen-Container ausnehmen
    const int xs[2] = { a.x1, a.x2 }, ys[2] = { a.y1, a.y2 };
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
      double dx = xs[i] - 179.5, dy = ys[j] - 179.5;
      if (dx * dx + dy * dy > 176.0 * 176.0) {
        printf("  ! %-12s ragt aus dem Kreis: %dx%d @ (%d,%d)-(%d,%d)\n",
               scenario, w, h, a.x1, a.y1, a.x2, a.y2);
        violations++;
        i = j = 2;
      }
    }
  }
  lv_area_t child_clip = a;
  if (lv_obj_has_flag(o, LV_OBJ_FLAG_OVERFLOW_VISIBLE)) child_clip = clip;
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(o); i++)
    check(lv_obj_get_child(o, i), scenario, child_clip);
}

static void scene(const char *name) {
  settle(400);
  shot(name);
  lv_mem_monitor_t mem;
  lv_mem_monitor(&mem);
  printf("[%s]  LVGL-Heap: %u B frei, %u %% belegt, %u %% fragmentiert\n",
         name, (unsigned)mem.free_size, (unsigned)mem.used_pct, (unsigned)mem.frag_pct);
  lv_area_t full = { 0, 0, SCR_W - 1, SCR_H - 1 };
  check(lv_scr_act(), name, full);
  check(lv_layer_top(), name, full);
}

int main() {
  lv_init();
  static lv_disp_draw_buf_t db;
  static lv_color_t buf[SCR_W * 40];
  lv_disp_draw_buf_init(&db, buf, nullptr, SCR_W * 40);
  static lv_disp_drv_t dd;
  lv_disp_drv_init(&dd);
  dd.hor_res = SCR_W; dd.ver_res = SCR_H; dd.flush_cb = flush_cb; dd.draw_buf = &db;
  lv_disp_drv_register(&dd);

  timers_init();
  ui_init();

  ui_splash_show();
  settle(200);
  shot("0-startbild");
  printf("[0-startbild]\n");
  settle(4200);          // Startbild auslaufen lassen - sonst liegt es ueber
                         // den naechsten Aufnahmen und die Anleitung zeigt
                         // ueberall das Logo statt der Seite

  scene("1-aktiv-leer");

  timer_start(20 * 60, 2, 0);          // Pasta 20:00
  timer_start(3 * 60 + 30, 1, 9);      // Ei 3:30
  timer_start(75 * 60, 17, 4);         // Backofen 1:15:00
  timer_toggle_pause(2);
  ui_active_update();
  scene("2-aktiv-drei");

  ui_goto(TILE_OVERVIEW);
  ui_overview_update();
  scene("2b-uebersicht");

  ui_goto(TILE_NEW);
  ui_new_load(0, 9, 16, -1);
  scene("3-neuer-timer");

  // Drehring-Kette pruefen: ui_on_knob -> Tile-Routing -> ui_new_knob -> Anzeige
  {
    ui_goto(TILE_NEW);
    settle(400);
    for (int i = 0; i < 7; i++) ui_on_knob(+1, 1);
    settle(200);
    shot("3b-neuer-timer-nach-7-rastungen");
    printf("[knob-kette] nach 7 Rastungen im Neu-Screen (Fokus %d) - Screenshot pruefen\n",
           ui_new_focus());
  }

  ui_goto(TILE_EGG);
  ui_egg_update();
  scene("3c-eieruhr");

  ui_goto(TILE_MEATER);
  ui_meater_update();
  scene("3e-fuehler");

  extern void sim_meater_state(MeaterState);
  sim_meater_state(MEATER_SEARCHING);    // ohne Fuehler: Logo statt Platzhalter
  ui_meater_update();
  scene("3g-fuehler-offline");
  sim_meater_state(MEATER_CONNECTED);

  setting_set_meater_armed(true);        // scharf: Zieltemperatur im Bild
  ui_meater_update();
  scene("3f-fuehler-ziel");

  ui_goto(TILE_STOPWATCH);
  ui_stopwatch_update();
  scene("3d-stoppuhr");

  // Stoppuhr laeuft mit: sie muss im Aktiv-Screen und in der Uebersicht auftauchen
  ui_stopwatch_toggle();
  usleep(300000);
  ui_goto(TILE_ACTIVE);
  ui_active_focus_stopwatch();
  ui_active_update();
  scene("2c-aktiv-stoppuhr");

  ui_goto(TILE_OVERVIEW);
  ui_overview_update();
  scene("2d-uebersicht-mit-stoppuhr");

  ui_goto(TILE_PRESETS);
  ui_presets_update();
  scene("4-vorlagen");

  ui_goto(TILE_SETTINGS);
  ui_settings_update();
  scene("5-einstellungen");

  ui_goto(TILE_ACTIVE);
  timer_start(1, 20, 3);                  // laeuft gleich ab -> Alarm
  usleep(1200000);
  timers_tick();
  ui_alarm_check();
  scene("6-alarm");

  ui_confirm("Timer loeschen?", nullptr, nullptr);
  scene("7-rueckfrage");

  printf("\n%s: %d Layoutverletzungen\n", violations ? "FEHLER" : "OK", violations);
  return violations ? 1 : 0;
}
