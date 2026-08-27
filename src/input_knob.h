#pragma once
#include <stdint.h>

void knob_init();
// Rastungen seit dem letzten Aufruf (+ = im Uhrzeigersinn), verbraucht den Zaehler.
int  knob_take_delta();
// Schrittweite passend zur Drehgeschwindigkeit: 1 bei langsam, 5/10 bei schnell.
int  knob_accel_step();
int32_t knob_raw();      // Zahl der Flanken (Diagnose)
int32_t knob_detents();  // gezaehlte Rastungen (Diagnose)
void knob_dump_trace();  // Flankenprotokoll auf Serial (Diagnose)
int  knob_pin_state();   // aktueller A/B-Pegel (Diagnose)
