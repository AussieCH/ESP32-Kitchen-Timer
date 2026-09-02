#pragma once
#include <stdint.h>
#include <stdbool.h>

// Der Alarm als eigene Sache: Blinken, Schnarren, Melodie und LED-Ring im
// gleichen Takt. Zwei Ausloeser teilen ihn sich - ein abgelaufener Timer und
// eine erreichte Zieltemperatur -, deshalb liegt er nicht mehr im Timer-Screen.
enum AlarmKind { ALARM_NONE, ALARM_TIMER, ALARM_TEMP };

void  alarm_init();                                   // einmal beim UI-Aufbau
void  alarm_start(uint32_t rgb, uint8_t melody, AlarmKind kind);
void  alarm_stop();
bool  alarm_active();
AlarmKind alarm_kind();
