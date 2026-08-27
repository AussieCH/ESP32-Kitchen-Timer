#pragma once
#include <lvgl.h>

#define ICON_COUNT 33

const char *icon_name(uint8_t id);      // Klartext fuer Buttons/Listen
const char *icon_abbrev(uint8_t id);    // Platzhalter, solange keine Grafiken da sind

// Erzeugt einen quadratischen Icon-Traeger. Sobald die echten Icons vorliegen,
// zeigt derselbe Traeger ein lv_img statt des Platzhaltertexts - die Screens
// muessen dafuer nicht angefasst werden.
lv_obj_t *icon_create(lv_obj_t *parent, int size);
void      icon_set(lv_obj_t *obj, uint8_t id, lv_color_t color);
