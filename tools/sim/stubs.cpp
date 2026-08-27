// Haptik und Backlight gibt es auf dem Host nicht - die UI ruft sie trotzdem.
#include "haptics.h"
void haptic_init() {}
void haptic_click() {}
void haptic_bump() {}
void haptic_buzz(bool) {}
bool haptic_present() { return true; }
void app_apply_brightness(int) {}
