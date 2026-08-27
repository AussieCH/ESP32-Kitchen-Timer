// LovyanGFX-Konfiguration fuer das 1.8"-Runddisplay (ST77916, QSPI, 360x360).
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "board_pins.h"

class LGFX_Knob : public lgfx::LGFX_Device {
  lgfx::Panel_ST77916 _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;

 public:
  LGFX_Knob() {
    { auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000UL;   // 80 MHz laeuft nicht auf allen Exemplaren stabil
      cfg.freq_read   = 16000000UL;
      cfg.spi_3wire   = true;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = (gpio_num_t)LCD_QSPI_SCL;
      cfg.pin_io0  = (gpio_num_t)LCD_QSPI_D0;
      cfg.pin_io1  = (gpio_num_t)LCD_QSPI_D1;
      cfg.pin_io2  = (gpio_num_t)LCD_QSPI_D2;
      cfg.pin_io3  = (gpio_num_t)LCD_QSPI_D3;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { auto cfg = _panel.config();
      cfg.pin_cs   = (gpio_num_t)LCD_QSPI_CS;
      cfg.pin_rst  = (gpio_num_t)LCD_RST;
      cfg.pin_busy = (gpio_num_t)-1;
      cfg.panel_width = cfg.memory_width  = 360;
      cfg.panel_height = cfg.memory_height = 360;
      cfg.offset_x = cfg.offset_y = cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable   = false;   // QSPI-Lesepfad ist nicht implementiert
      cfg.invert     = true;    // Init-Sequenz sendet INVON -> hier gegenkompensieren
      cfg.rgb_order  = true;    // bei Rot/Blau-Tausch umdrehen
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    { auto cfg = _light.config();
      cfg.pin_bl      = (gpio_num_t)LCD_BLK;
      cfg.invert      = false;
      cfg.freq        = 20000;  // gegen das Flackern zwischen 40-70 % Duty
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
