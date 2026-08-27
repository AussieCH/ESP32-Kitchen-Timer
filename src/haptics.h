#pragma once
#include <stdint.h>

void haptic_init();
void haptic_click();          // kurze Rastung beim Drehen
void haptic_bump();           // kraeftiger Puls fuer Bestaetigungen
void haptic_buzz(bool on);    // Dauervibration fuer den Alarm (RTP-Modus)
bool haptic_present();
