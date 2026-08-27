// Nur so viel Arduino, wie die UI-/Timer-Dateien auf dem Host brauchen.
// Achtung: LVGLs LV_TICK_CUSTOM_INCLUDE zieht diese Datei auch aus C-Quellen,
// also muss der C++-Teil hinter __cplusplus liegen.
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>

static inline uint32_t millis(void) {
  struct timeval tv; gettimeofday(&tv, NULL);
  static long t0 = 0;
  long now = tv.tv_sec * 1000L + tv.tv_usec / 1000;
  if (!t0) t0 = now;
  return (uint32_t)(now - t0);
}
static inline void delay(uint32_t ms) { (void)ms; }

#ifdef __cplusplus
#include <algorithm>
using std::max;
using std::min;

template <typename T> static inline T constrain(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct SerialStub {
  void begin(int) {}
  void println(const char *s = "") { ::printf("%s\n", s); }
  template <typename... A> void printf(const char *f, A... a) { ::printf(f, a...); }
};
static SerialStub Serial;
#endif
