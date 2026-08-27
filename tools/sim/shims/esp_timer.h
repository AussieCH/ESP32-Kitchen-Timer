#pragma once
#include <stdint.h>
#include <sys/time.h>
static inline int64_t esp_timer_get_time() {
  struct timeval tv; gettimeofday(&tv, nullptr);
  static int64_t t0 = 0;
  int64_t now = (int64_t)tv.tv_sec * 1000000LL + tv.tv_usec;
  if (!t0) t0 = now;
  return now - t0;
}
