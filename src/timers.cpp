#include <Arduino.h>
#include <Preferences.h>
#include <esp_timer.h>
#include "timers.h"
#include "audio.h"

static ActiveTimer s_act[MAX_ACTIVE];
static ActiveTimer *s_sorted[MAX_ACTIVE];
static int s_sorted_n = 0;
static uint32_t s_next_id = 1;
static uint8_t s_next_color = 0;

static Preset s_presets[MAX_PRESETS];
static int s_preset_n = 0;

static int s_brightness = 80;
static int s_meater_target = 58;
static bool s_meater_armed = false;
static Preferences s_prefs;

int64_t now_ms() { return esp_timer_get_time() / 1000; }

// ---------------------------------------------------------------- Persistenz
static void presets_save() {
  s_prefs.putBytes("presets", s_presets, sizeof(Preset) * s_preset_n);
  s_prefs.putInt("preset_n", s_preset_n);
}

void settings_save() {
  s_prefs.putInt("bright", s_brightness);
  s_prefs.putInt("vol", audio_get_volume());
  s_prefs.putInt("mtarget", s_meater_target);
  s_prefs.putBool("marmed", s_meater_armed);
}

void timers_init() {
  memset(s_act, 0, sizeof(s_act));
  s_prefs.begin("ktimer", false);

  s_preset_n = s_prefs.getInt("preset_n", 0);
  if (s_preset_n < 0 || s_preset_n > MAX_PRESETS) s_preset_n = 0;
  if (s_preset_n) s_prefs.getBytes("presets", s_presets, sizeof(Preset) * s_preset_n);

  s_brightness = constrain(s_prefs.getInt("bright", 80), 10, 100);
  audio_set_volume(s_prefs.getInt("vol", 70));
  s_meater_target = constrain(s_prefs.getInt("mtarget", 58), 30, 99);
  s_meater_armed = s_prefs.getBool("marmed", false);

  // Icon-IDs stecken in den Vorlagen: aendert sich der Icon-Satz, sind gespeicherte
  // Vorlagen wertlos - deshalb eine Version im NVS.
  const int PRESET_VERSION = 5;
  if (s_prefs.getInt("pver", 0) != PRESET_VERSION) {
    s_preset_n = 0;
    s_prefs.putInt("pver", PRESET_VERSION);
  }

  if (s_preset_n == 0) {              // Erststart: ein paar sinnvolle Vorlagen
    preset_remember( 8 * 60,  0, 9);   // Pasta
    preset_remember(20 * 60,  4, 4);   // Kartoffeln
    preset_remember( 5 * 60,  1, 0);   // Ei
    preset_remember( 3 * 60, 29, 18);  // Wasserkessel
  }
}

int setting_brightness() { return s_brightness; }
void setting_set_brightness(int v) { s_brightness = constrain(v, 10, 100); }
int  setting_meater_target() { return s_meater_target; }
void setting_set_meater_target(int c) { s_meater_target = constrain(c, 30, 99); }
bool setting_meater_armed() { return s_meater_armed; }
void setting_set_meater_armed(bool on) { s_meater_armed = on; }

// ---------------------------------------------------------------- laufende Timer
uint32_t timer_remaining_ms(const ActiveTimer *t) {
  if (!t || !t->used) return 0;
  if (t->paused) return t->rest_ms;
  int64_t left = t->end_ms - now_ms();
  return left > 0 ? (uint32_t)left : 0;
}

static void resort() {
  s_sorted_n = 0;
  for (int i = 0; i < MAX_ACTIVE; i++)
    if (s_act[i].used) s_sorted[s_sorted_n++] = &s_act[i];

  // klingelnde zuerst, dann nach Restzeit; stabil ueber die ID
  for (int i = 1; i < s_sorted_n; i++) {
    ActiveTimer *k = s_sorted[i];
    int j = i - 1;
    while (j >= 0) {
      ActiveTimer *a = s_sorted[j];
      bool swap = false;
      int ra = (a->ringing || a->expired) ? 0 : 1;
      int rk = (k->ringing || k->expired) ? 0 : 1;
      if (rk < ra) swap = true;
      else if (rk == ra && timer_remaining_ms(k) < timer_remaining_ms(a)) swap = true;
      if (!swap) break;
      s_sorted[j + 1] = a; j--;
    }
    s_sorted[j + 1] = k;
  }
}

int active_count() { return s_sorted_n; }
ActiveTimer *active_at(int idx) { return (idx >= 0 && idx < s_sorted_n) ? s_sorted[idx] : nullptr; }

int timer_start(uint32_t total_s, uint8_t icon, uint8_t melody) {
  if (total_s == 0) return -1;
  for (int i = 0; i < MAX_ACTIVE; i++) {
    if (s_act[i].used) continue;
    ActiveTimer *t = &s_act[i];
    memset(t, 0, sizeof(*t));
    t->used = true;
    t->id = s_next_id++;
    t->total_s = total_s;
    t->end_ms = now_ms() + (int64_t)total_s * 1000;
    t->icon = icon; t->melody = melody;
    // Farbe, die gerade kein anderer laufender Timer hat - erst wenn alle acht
    // vergeben sind, faengt es von vorne an. Die Farbe ist die Identitaet des
    // Timers, auf dem Schirm wie auf dem LED-Ring.
    bool used[8] = { false };
    for (int k = 0; k < MAX_ACTIVE; k++)
      if (s_act[k].used && &s_act[k] != t) used[s_act[k].color % 8] = true;
    t->color = s_next_color % 8;
    for (int c = 0; c < 8; c++) {
      int cand = (s_next_color + c) % 8;
      if (!used[cand]) { t->color = cand; break; }
    }
    s_next_color = (uint8_t)((t->color + 1) % 8);
    resort();
    for (int k = 0; k < s_sorted_n; k++) if (s_sorted[k] == t) return k;
    return 0;
  }
  return -1;   // alle Plaetze belegt
}

void timer_toggle_pause(int idx) {
  ActiveTimer *t = active_at(idx);
  if (!t || t->ringing || t->expired) return;
  if (t->paused) {
    t->end_ms = now_ms() + t->rest_ms;
    t->paused = false;
  } else {
    t->rest_ms = timer_remaining_ms(t);
    t->paused = true;
  }
  resort();
}

void timer_add_seconds(int idx, int32_t secs) {
  ActiveTimer *t = active_at(idx);
  if (!t) return;
  if (t->ringing || t->expired) {   // abgelaufener Timer: Alarm aus, neu anlaufen lassen
    t->ringing = t->expired = false;
    t->paused = false;
    t->end_ms = now_ms() + (int64_t)secs * 1000;
    t->total_s = secs;
  } else if (t->paused) {
    int64_t r = (int64_t)t->rest_ms + (int64_t)secs * 1000;
    t->rest_ms = r > 0 ? (uint32_t)r : 0;
  } else {
    t->end_ms += (int64_t)secs * 1000;
  }
  t->total_s = (uint32_t)max<int64_t>(t->total_s, timer_remaining_ms(t) / 1000);
  resort();
}

void timer_delete(int idx) {
  ActiveTimer *t = active_at(idx);
  if (!t) return;
  t->used = false;
  resort();
}

void timer_ack(int idx) { timer_delete(idx); }

void timer_snooze(int idx, uint32_t secs) {
  ActiveTimer *t = active_at(idx);
  if (!t) return;
  t->ringing = t->expired = false;
  t->paused = false;
  t->total_s = secs;
  t->end_ms = now_ms() + (int64_t)secs * 1000;
  resort();
}

bool timers_tick() {
  bool new_alarm = false;
  int64_t now = now_ms();
  for (int i = 0; i < MAX_ACTIVE; i++) {
    ActiveTimer *t = &s_act[i];
    if (!t->used || t->paused || t->expired) continue;
    if (!t->ringing && now >= t->end_ms) {
      t->ringing = true;
      t->ring_start_ms = now;
      new_alarm = true;
    } else if (t->ringing && now - t->ring_start_ms > RING_TIMEOUT_MS) {
      t->ringing = false;      // Ton verstummt, Timer bleibt als "abgelaufen" stehen
      t->expired = true;
    }
  }
  resort();
  return new_alarm;
}

int timer_first_ringing() {
  for (int i = 0; i < s_sorted_n; i++) if (s_sorted[i]->ringing) return i;
  return -1;
}

int ringing_count() {
  int n = 0;
  for (int i = 0; i < s_sorted_n; i++) if (s_sorted[i]->ringing) n++;
  return n;
}

// ---------------------------------------------------------------- Vorlagen
int preset_count() { return s_preset_n; }
Preset *preset_at(int idx) { return (idx >= 0 && idx < s_preset_n) ? &s_presets[idx] : nullptr; }

void preset_remember(uint32_t total_s, uint8_t icon, uint8_t melody) {
  for (int i = 0; i < s_preset_n; i++) {          // schon vorhanden? nur nach oben schieben
    if (s_presets[i].total_s == total_s && s_presets[i].icon == icon) {
      s_presets[i].melody = melody;
      Preset p = s_presets[i];
      for (int j = i; j > 0; j--) s_presets[j] = s_presets[j - 1];
      s_presets[0] = p;
      presets_save();
      return;
    }
  }
  if (s_preset_n < MAX_PRESETS) s_preset_n++;     // sonst faellt die aelteste raus
  for (int j = s_preset_n - 1; j > 0; j--) s_presets[j] = s_presets[j - 1];
  s_presets[0] = { total_s, icon, melody };
  presets_save();
}

void preset_replace(int idx, uint32_t total_s, uint8_t icon, uint8_t melody) {
  if (idx < 0 || idx >= s_preset_n) return;
  s_presets[idx] = { total_s, icon, melody };
  presets_save();
}

void preset_delete(int idx) {
  if (idx < 0 || idx >= s_preset_n) return;
  for (int j = idx; j < s_preset_n - 1; j++) s_presets[j] = s_presets[j + 1];
  s_preset_n--;
  presets_save();
}
