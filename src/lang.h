// Sprachumschaltung Deutsch / Englisch.
//
// Alle Anzeigetexte laufen ueber T(...). Die Reihenfolge der Kennungen hier und
// der Eintraege in lang.cpp muss uebereinstimmen - eine Zusicherung dort prueft
// wenigstens die Anzahl.
#pragma once
#include <stdint.h>

enum Lang { LANG_DE = 0, LANG_EN = 1, LANG_COUNT };

enum TxtId {
  // Ueberschriften
  T_OVERVIEW, T_NEW_TIMER, T_EGG, T_MEATER, T_STOPWATCH, T_TEMPLATES, T_SETTINGS,
  // Knoepfe
  T_PAUSE, T_RESUME, T_STOP, T_START, T_START_IT, T_EDIT, T_DELETE, T_RESET,
  T_COOK, T_TEST_ALARM, T_ALARM_ON, T_ALARM_OFF, T_YES, T_NO, T_PLUS1, T_PLUS5,
  // Kurzmeldungen
  T_TIMER_STARTED, T_TIMER_DELETED, T_TEMPLATE_DELETED, T_PLUS1_MSG, T_PLUS5_MSG,
  T_SET_TIME, T_TOO_MANY, T_EGG_RUNNING, T_ARMED, T_DISARMED, T_ALARM_STOPPED,
  T_NO_HAPTIC,
  // Rueckfragen
  T_Q_DELETE_TIMER, T_Q_DELETE_TEMPLATE,
  // Zustaende
  T_EXPIRED, T_PAUSED, T_DONE, T_NO_TIMER, T_SEARCHING, T_OFF, T_BG_RUNNING,
  T_TARGET_REACHED, T_PROBE_HINT, T_NO_TEMPLATES, T_HINT_LONGPRESS, T_HINT_KNOB,
  // Einstellungen
  T_BRIGHTNESS, T_VOLUME, T_LANGUAGE, T_PROBE_BATT,
  // Eieruhr
  T_SIZE, T_START_TEMP, T_RESULT, T_FRIDGE, T_ROOM, T_SOFT, T_MEDIUM_EGG, T_HARD,
  // Garstufen
  T_RARE, T_ENGLISH, T_PINK, T_MEDIUM, T_HALF_DONE, T_WELL_DONE, T_VERY_DONE,
  // Thermometer
  T_TARGET, T_CHAMBER,
  TXT_COUNT
};

const char *T(TxtId id);
Lang        lang_get();
void        lang_set(Lang l);
uint8_t     lang_rev();            // steigt bei jedem Wechsel - Screens zeichnen neu
const char *lang_label(Lang l);    // "Deutsch" / "English"

// Symbol + uebersetzter Text fuer Knopfbeschriftungen, z.B. Play + " Start".
// Liefert einen von mehreren Ringpuffern - fuer einen Knopf reicht das, der
// Text wird beim Setzen ohnehin kopiert.
const char *lang_btn(const char *symbol, TxtId id);
