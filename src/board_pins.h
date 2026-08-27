// Pinout schaltplanverifiziert: Waveshare ESP32-S3-Knob-Touch-LCD-1.8
// (baugleich Guition K5 / JC3636K518). NICHT mit dem JC3636K718C mischen.
#pragma once

// ---- Display ST77916, QSPI, 360x360 ---------------------------------------
#define LCD_QSPI_SCL   13
#define LCD_QSPI_CS    14
#define LCD_QSPI_D0    15
#define LCD_QSPI_D1    16
#define LCD_QSPI_D2    17
#define LCD_QSPI_D3    18
#define LCD_RST        21
#define LCD_BLK        47   // aktiv HIGH (NMOS-Gate); LCD_TE liegt auf keinem GPIO

// ---- Touch CST816 + Haptik DRV2605 am selben I2C-Bus ----------------------
#define TP_SDA         11
#define TP_SCL         12
#define TP_INT          9
#define TP_RST         10
#define CST816_ADDR  0x15
#define DRV2605_ADDR 0x5A

// ---- Drehencoder EC1 (S3-Seite), kein Taster im Knopf ---------------------
#define EC1_A           8
#define EC1_B           7

// ---- Audio ----------------------------------------------------------------
#define I2S_DAC_BCK    39
#define I2S_DAC_LRCK   40
#define I2S_DAC_DIN    41
#define I2S_SWITCH_IN   0   // CH445P: HIGH = S3 am DAC. BOOT-Pin -> erst nach dem Start treiben!
// XSMT (Mute des PCM5100A) haengt an ESP32-U4WDH IO32 und ist vom S3 aus NICHT
// erreichbar. Ohne HIGH dort bleibt der DAC stumm -> siehe README, Schritt 1.

#define BATT_ADC        1   // ADC1_CH0, 10k/10k am 5V-Rail -> Wert x2 (System-, nicht Zellspannung)

// ---- Drehrichtung ---------------------------------------------------------
// Erste Zwischenstufe nach dem Verlassen der Ruhelage (AB=3), die als
// "im Uhrzeigersinn" gilt: 1 = A faellt zuerst, 2 = B faellt zuerst.
#define KNOB_FIRST_CW   1
#define KNOB_INVERT     0   // auf 1, falls der Ring gefuehlt verkehrt herum zaehlt

// ---- Touch-Kalibrierung (am Geraet einmal verifizieren) -------------------
#define TP_SWAP_XY      0
#define TP_MIRROR_X     0
#define TP_MIRROR_Y     0
