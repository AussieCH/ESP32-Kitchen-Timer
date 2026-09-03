/* LVGL 8.4 - Minimalkonfiguration.
 * Alles was hier fehlt, faellt in lv_conf_internal.h auf die Defaults zurueck. */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0      /* Byte-Swap macht LovyanGFX beim Push */

#define LV_MEM_CUSTOM           0
// 64 KB reichten bis fuenf Screens. Mit acht Kacheln und den Masken des
// Alarm-Overlays schlaegt lv_mem_alloc fehl - und LVGLs Assert-Handler ist ein
// while(1), das Geraet steht dann einfach. Im Simulator reproduzierbar.
// 96 KB statt 128: der BLE-Stack braucht rund 100 KB internen Speicher, und
// der Spitzenverbrauch von LVGL liegt laut Simulator bei 64 KB.
#define LV_MEM_SIZE             (96U * 1024U)

#define LV_DISP_DEF_REFR_PERIOD 20
#define LV_INDEV_DEF_READ_PERIOD 15

/* Gestenschwelle und Long-Press-Zeit sind in LVGL 8.4 fest in lv_hal_indev.h
 * verdrahtet (50 px / 400 ms) - hier nicht konfigurierbar. */

#define LV_TICK_CUSTOM          1
#define LV_TICK_CUSTOM_INCLUDE  "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0
#define LV_USE_LOG              0

// Eigene Oberflaechenschrift statt der eingebauten: Montserrat enthaelt nur
// ASCII (0x20-0x7F), ein "ü" ist dort nicht vorhanden. font_ui_* ist Rubik mit
// Umlauten plus denselben Symbolen - erzeugt von tools/make_ui_fonts.py.
// Die eingebauten Schriften bleiben aus, sonst laegen beide im Flash.
#define LV_FONT_MONTSERRAT_14   0
#define LV_FONT_MONTSERRAT_16   0
#define LV_FONT_MONTSERRAT_20   0
#define LV_FONT_MONTSERRAT_24   0
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_CUSTOM_DECLARE  LV_FONT_DECLARE(font_ui_16);
#define LV_FONT_DEFAULT         &font_ui_16

#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1

#endif /*LV_CONF_H*/
