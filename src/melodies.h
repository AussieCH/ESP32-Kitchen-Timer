// Alarmmelodien als {Frequenz, Dauer}-Tabellen (Daten in melodies.cpp).
// Nur fuer Boards mit Lautsprecher - der Waveshare-Knob hat keinen.
//
// Eigene Melodie: in melodies.cpp eine Tabelle anlegen und unten in MELODIES
// eintragen. REST = Pause. Der Kurzname sollte <= 13 Zeichen haben, sonst wird
// er auf dem Button abgeschnitten; Umlaute kann der eingebaute Font nicht.
#pragma once
#include <stdint.h>

#define REST 0

struct Note   { uint16_t freq; uint16_t ms; };
struct Melody { const char *de; const char *en; const Note *notes; uint16_t count; };

// Name in der eingestellten Sprache
const char *melody_name(const Melody *m);

extern const Melody MELODIES[];
extern const int    MELODY_COUNT;
extern const Melody MELODY_TEST;
