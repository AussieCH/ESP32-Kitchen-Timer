#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MAX_ACTIVE   8
#define MAX_PRESETS  20
#define RING_TIMEOUT_MS (5 * 60 * 1000)   // danach verstummt der Alarm von selbst

struct ActiveTimer {
  bool     used;
  uint32_t id;
  uint32_t total_s;
  int64_t  end_ms;        // monotone Zielzeit - NICHT pro Tick herunterzaehlen (Drift)
  uint32_t rest_ms;       // gueltig, solange paused
  bool     paused;
  bool     ringing;       // klingelt gerade
  bool     expired;       // abgelaufen, Ton beendet, noch nicht quittiert
  int64_t  ring_start_ms;
  uint8_t  icon, melody, color;
};

struct Preset {
  uint32_t total_s;
  uint8_t  icon, melody;      // melody nur auf Boards mit Lautsprecher benutzt
};

void timers_init();
int64_t now_ms();

// --- laufende Timer ---------------------------------------------------------
int   active_count();
ActiveTimer *active_at(int idx);              // idx 0..active_count()-1, nach Restzeit sortiert
int   timer_start(uint32_t total_s, uint8_t icon, uint8_t melody);
void  timer_toggle_pause(int idx);
void  timer_add_seconds(int idx, int32_t secs);
void  timer_delete(int idx);
uint32_t timer_remaining_ms(const ActiveTimer *t);

// true, sobald ein Timer neu abgelaufen ist (UI zieht dann das Alarm-Overlay hoch)
bool  timers_tick();
int   timer_first_ringing();                  // -1 = keiner
int   ringing_count();
void  timer_ack(int idx);                     // Alarm quittieren -> Timer weg
void  timer_snooze(int idx, uint32_t secs);   // "+5 Min" aus dem Alarm heraus

// --- Vorlagen ---------------------------------------------------------------
int   preset_count();
Preset *preset_at(int idx);
void  preset_remember(uint32_t total_s, uint8_t icon, uint8_t melody);  // legt an oder schiebt nach oben
void  preset_replace(int idx, uint32_t total_s, uint8_t icon, uint8_t melody);
void  preset_delete(int idx);

// --- Einstellungen ----------------------------------------------------------
int   setting_brightness();       // 10..100
void  setting_set_brightness(int v);
void  settings_save();            // Helligkeit + Lautstaerke (entprellt aufrufen)
