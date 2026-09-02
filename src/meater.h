#pragma once
#include <stdbool.h>
#include <stdint.h>

// Anbindung an einen MEATER-Fleischfuehler (Bluetooth Low Energy).
//
// Der Fuehler laesst nur EINE Verbindung zu: solange RONDO verbunden ist,
// sieht die MEATER-App nichts - und umgekehrt.
enum MeaterState { MEATER_OFF, MEATER_SEARCHING, MEATER_CONNECTED };

void  meater_init();
MeaterState meater_state();
float meater_tip_c();          // Kerntemperatur
float meater_ambient_c();      // Umgebung (Garraum)
int   meater_battery();        // Prozent, -1 = unbekannt
uint32_t meater_age_ms();      // wie alt ist der letzte Messwert
const char *meater_name();
