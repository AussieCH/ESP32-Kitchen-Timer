// Waehlt den Pinout des Zielboards. Beide Boards haben denselben Display-
// Controller (ST77916, 360x360, QSPI) und denselben Touch (CST816) - sonst ist
// so gut wie nichts gleich. Niemals Pins zwischen den Headern kopieren.
#pragma once

#if defined(BOARD_GUITION_K718)
  #include "boards/guition_k718.h"
#elif defined(BOARD_WAVESHARE_KNOB)
  #include "boards/waveshare_knob.h"
#else
  #error "Kein Board gewaehlt - -D BOARD_WAVESHARE_KNOB oder -D BOARD_GUITION_K718 setzen"
#endif
