#pragma once
#include <stdbool.h>
#include <stdint.h>

void touch_init();
// true = Finger auf dem Panel; x/y in Displaykoordinaten
bool touch_read(int16_t *x, int16_t *y);
