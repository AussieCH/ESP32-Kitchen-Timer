// Tonausgabe ueber den PCM5100A (Guition JC3636K718C, mit eingebautem Lautsprecher).
//
// Zwei Dinge sind hier anders als auf dem Waveshare-Knob: der DAC haengt am
// selben Chip wie das UI (kein I2S-Umschalter), und das Mute-Signal liegt auf
// GPIO46 dieses Chips - LOW ist stumm, also dauerhaft HIGH halten. GPIO46 ist
// ein Strappingpin, wir treiben ihn erst hier, lange nach dem Reset.
#include "audio.h"


#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#define SAMPLE_RATE   22050
#define BLOCK_FRAMES  256

static I2SClass s_i2s;
static bool s_ok = false;

static volatile int  s_volume  = 60;
static volatile bool s_stop    = false;
static volatile bool s_playing = false;
static volatile bool s_repeat  = false;
static volatile bool s_fade_in = false;
static const Melody *volatile s_req = nullptr;

static float s_phase = 0.0f;
static int16_t s_block[BLOCK_FRAMES * 2];
static uint32_t s_play_start_ms = 0;

static float current_gain() {
  float g = s_volume / 100.0f;
  g = g * g;                                   // gefuehlt linearer als roh
  if (s_fade_in) {                             // ueber 8 s von 25 % auf 100 %
    float t = (millis() - s_play_start_ms) / 8000.0f;
    if (t > 1.0f) t = 1.0f;
    g *= 0.25f + 0.75f * t;
  }
  return g * 0.85f;
}

// Ton mit weicher Flanke, damit es an den Notengrenzen nicht knackt.
static void render(float freq, uint32_t ms) {
  if (!s_ok) { delay(ms); return; }
  uint32_t total = (uint32_t)((uint64_t)ms * SAMPLE_RATE / 1000);
  uint32_t done = 0;
  const uint32_t ramp = SAMPLE_RATE / 200;     // 5 ms
  const float step = freq > 0 ? 2.0f * (float)M_PI * freq / SAMPLE_RATE : 0.0f;

  while (done < total) {
    if (s_stop) return;
    uint32_t n = total - done; if (n > BLOCK_FRAMES) n = BLOCK_FRAMES;
    float gain = current_gain();

    for (uint32_t i = 0; i < n; i++) {
      float env = 1.0f;
      uint32_t pos = done + i;
      if (pos < ramp)               env = (float)pos / ramp;
      else if (total - pos < ramp)  env = (float)(total - pos) / ramp;

      int16_t v = 0;
      if (freq > 0) {
        s_phase += step;
        if (s_phase > 2.0f * (float)M_PI) s_phase -= 2.0f * (float)M_PI;
        // Grundton + etwas dritte Harmonische: traegt besser durch Kuechenlaerm
        float s = sinf(s_phase) + 0.25f * sinf(3.0f * s_phase);
        v = (int16_t)(s * 12000.0f * gain * env);
      }
      s_block[i * 2] = v; s_block[i * 2 + 1] = v;
    }
    s_i2s.write((const uint8_t *)s_block, n * 2 * sizeof(int16_t));
    done += n;
  }
}

static void play_melody(const Melody *m) {
  for (uint16_t i = 0; i < m->count && !s_stop; i++) {
    uint32_t ms = m->notes[i].ms;
    render(m->notes[i].freq, (ms * 92) / 100);   // Rest als Luft zwischen den Noten
    if (!s_stop) render(0, ms - (ms * 92) / 100);
  }
}

static void audio_task(void *) {
  for (;;) {
    const Melody *req = s_req;
    if (req) {
      s_playing = true;
      s_play_start_ms = millis();
      do {
        play_melody(req);
        if (s_repeat && !s_stop) render(0, 700);   // Atempause vor der Wiederholung
      } while (s_repeat && !s_stop && s_req == req);
      s_playing = false;
      if (s_req == req) s_req = nullptr;
      s_stop = false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void audio_init() {
  pinMode(I2S_DAC_MUTE, OUTPUT);
  digitalWrite(I2S_DAC_MUTE, HIGH);            // LOW waere stumm
  delay(5);

  s_i2s.setPins(I2S_DAC_BCK, I2S_DAC_LRCK, I2S_DAC_DIN, -1, -1);  // kein MCLK
  s_ok = s_i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  Serial.printf("[audio] I2S %s\n", s_ok ? "ok" : "FEHLER");

  xTaskCreatePinnedToCore(audio_task, "audio", 4096, nullptr, 2, nullptr, 0);
}

bool audio_present()    { return s_ok; }
void audio_set_volume(int vol) { s_volume = constrain(vol, 0, 100); }
int  audio_get_volume() { return s_volume; }
bool audio_is_playing() { return s_playing || s_req != nullptr; }

void audio_play(const Melody *m, bool repeat) {
  audio_stop();
  s_repeat = repeat;
  s_req = m;
}

void audio_stop() {
  if (s_req || s_playing) {
    s_stop = true;
    s_req = nullptr;
    uint32_t t0 = millis();
    while (s_playing && millis() - t0 < 200) delay(5);
    s_stop = false;
  }
  s_fade_in = false;
}

void audio_set_fade_in(bool on) { s_fade_in = on; s_play_start_ms = millis(); }

