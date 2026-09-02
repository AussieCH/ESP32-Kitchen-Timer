#pragma once
#include <stdbool.h>
#include <stdint.h>

// Akku- bzw. Systemspannung. Beide Boards messen ueber einen 10k/10k-Teiler,
// aber NICHT dasselbe: beim Guition haengt er an der Zelle, beim Waveshare am
// 5V-Rail (Waveshare nennt es selbst "System Voltage"). Deshalb gibt es
// battery_is_cell() - eine Prozentzahl auf eine Railspannung waere gelogen.
void  battery_init();
void  battery_tick();          // ~1x pro Sekunde aufrufen
float battery_volts();
int   battery_percent();       // 0..100, nur sinnvoll wenn battery_is_cell()
bool  battery_is_cell();       // false = haengt an USB / am Rail
bool  battery_low();           // < 15 % auf Akku
