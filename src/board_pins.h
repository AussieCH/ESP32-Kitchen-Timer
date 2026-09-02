// Pinout Guition JC3636K718C ("K/Knob"-Serie), aus dem Herstellerdemo
// JC3636K718_knob_EN (pincfg.h / scr_st77916.h) und am Geraet nachgeprueft.
// ACHTUNG: unterscheidet sich komplett vom JC3636W518 - Display, Backlight und
// Touch liegen alle woanders. Pins anderer Boards niemals hierher kopieren.
#pragma once

#define BOARD_NAME       "Guition JC3636K718C"
// Am Geraet nachgemessen: der I2C-Scan beim Start findet 0x15 (Touch) UND 0x5A,
// der DRV2605 antwortet. Weder das Herstellerdemo noch die kursierenden Pinouts
// erwaehnen ihn; bestueckt ist er trotzdem.

// ---- Display ST77916, QSPI, 360x360 ---------------------------------------
#define LCD_QSPI_SCL   11
#define LCD_QSPI_CS    12
#define LCD_QSPI_D0    13
#define LCD_QSPI_D1    14
#define LCD_QSPI_D2    15
#define LCD_QSPI_D3    16
#define LCD_RST        17
#define LCD_BLK        21   // PWM

// ---- Touch CST816 (I2C, interne Pullups) ----------------------------------
#define TP_SDA          9
#define TP_SCL         10
#define TP_INT          7
#define TP_RST          8
#define CST816_ADDR  0x15
#define DRV2605_ADDR 0x5A   // falls bestueckt; wird beim Start geprueft

// ---- Drehknopf: zwei getrennte Leitungen, kein Encoder --------------------
// Pro Rastung ein sauberer LOW-Impuls - links auf GPIO2, rechts auf GPIO1.
// Ein PCNT-Encoder liest hier NULL. Der Knopf ist nicht drueckbar.
#define KNOB_PIN_LEFT   2
#define KNOB_PIN_RIGHT  1
#define KNOB_INVERT     0
// Am Geraet gemessen: der Knopf pulst ZWEIMAL pro Rastung - einmal auf halbem
// Weg, einmal beim Einrasten. Ohne Teiler springt die Anzeige beim halben Klick.
#define KNOB_PULSES_PER_DETENT 2

// ---- Audio PCM5100A (I2S) -------------------------------------------------
#define I2S_DAC_BCK     3
#define I2S_DAC_LRCK   45
#define I2S_DAC_DIN    42
#define I2S_DAC_MUTE   46   // LOW = stumm -> dauerhaft HIGH halten (Strappingpin)

// ---- WS2812-Ring ----------------------------------------------------------
#define LED_RING_PIN    0   // zugleich BOOT-Strappingpin: erst nach dem Start treiben
#define LED_RING_COUNT 13

// ---- Sonstiges ------------------------------------------------------------
#define BATT_ADC        6   // Teiler ~10k/10k -> x2, Prozentkurve ist Heuristik

// ---- Touch-Kalibrierung (am Geraet einmal verifizieren) -------------------
#define TP_SWAP_XY      0
#define TP_MIRROR_X     0
#define TP_MIRROR_Y     0
