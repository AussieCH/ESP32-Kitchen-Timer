#pragma once
#include <lvgl.h>
#include "timers.h"
#include "icons.h"

#define SCR_W 360
#define SCR_H 360

// Reihenfolge von links nach rechts. Die Uebersicht liegt links vom Aktiv-
// Screen: nach rechts wischen (Inhalt wandert nach rechts) fuehrt dorthin.
#define TILE_OVERVIEW 0
#define TILE_ACTIVE   1
#define TILE_NEW      2
#define TILE_PRESETS  3
#define TILE_SETTINGS 4
#define TILE_COUNT    5

LV_FONT_DECLARE(font_time_92);
LV_FONT_DECLARE(font_time_52);
LV_FONT_DECLARE(font_time_30);

lv_color_t timer_color(uint8_t idx);
lv_color_t col_dim();

void ui_init();
void ui_tick();                       // 4x pro Sekunde aus dem Hauptloop
void ui_on_knob(int delta, int step);
void ui_goto(int tile);
int  ui_current_tile();

void ui_toast(const char *txt);
int  ui_new_focus();
void ui_debug_set(const char *txt);
lv_obj_t *make_button(lv_obj_t *parent, const char *txt, lv_coord_t w, lv_coord_t h,
                      lv_event_cb_t cb, void *user);
void      button_set_color(lv_obj_t *btn, lv_color_t c);
void      button_set_text(lv_obj_t *btn, const char *txt);
void      fmt_time(char *buf, size_t n, uint32_t secs);       // 12:34 bzw. 1:02:03
typedef void (*ui_yes_cb_t)(void *user);
void      app_apply_brightness(int pct);   // in main.cpp
void      ui_confirm(const char *question, ui_yes_cb_t yes_cb, void *user);

// --- Screens ---------------------------------------------------------------
void ui_overview_create(lv_obj_t *p); void ui_overview_update();
void ui_overview_knob(int d, int step);

void ui_active_create(lv_obj_t *p);   void ui_active_update();
void ui_active_focus_id(uint32_t id);
void ui_active_knob(int d, int step); void ui_active_gesture(lv_dir_t dir);

void ui_new_create(lv_obj_t *p);      void ui_new_update();
void ui_new_knob(int d, int step);
void ui_new_load(uint32_t total_s, uint8_t icon, uint8_t melody, int edit_preset_idx);

void ui_presets_create(lv_obj_t *p);  void ui_presets_update();
void ui_presets_knob(int d, int step);

void ui_settings_create(lv_obj_t *p); void ui_settings_update();
void ui_settings_knob(int d, int step);

// Alarm laeuft im Aktiv-Screen: er kommt nach vorn, blinkt und laesst die
// Haptik schnarren - Ton gibt es auf diesem Board nicht (nur Line-Out).
void ui_alarm_check();
