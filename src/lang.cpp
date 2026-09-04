// Textkatalog. Links Deutsch, rechts Englisch - in der Reihenfolge von TxtId.
#include "lang.h"
#include <stdio.h>

static Lang s_lang = LANG_DE;
static uint8_t s_rev = 0;

static const char *TXT[TXT_COUNT][LANG_COUNT] = {
  // Ueberschriften
  { "Läuft gerade",      "Running now" },
  { "Neuer Timer",       "New timer" },
  { "Eieruhr",           "Egg timer" },
  { "Grill-Thermometer", "Grill thermometer" },
  { "Stoppuhr",          "Stopwatch" },
  { "Vorlagen",          "Presets" },
  { "Einstellungen",     "Settings" },
  // Knoepfe
  { " Pause",            " Pause" },
  { " Weiter",           " Resume" },
  { " Stopp",            " Stop" },
  { " Start",            " Start" },
  { " Starten",          " Start" },
  { " Editieren",        " Edit" },
  { " Löschen",          " Delete" },
  { "Zurück",            "Reset" },
  { " Kochen",           " Cook" },
  { " Alarm testen",     " Test alarm" },
  { "Alarm ein",         "Alarm on" },
  { "Alarm aus",         "Alarm off" },
  { "Ja",                "Yes" },
  { "Nein",              "No" },
  { "+1 Min",            "+1 min" },
  { "+5 Min",            "+5 min" },
  // Kurzmeldungen
  { "Timer gestartet",   "Timer started" },
  { "Timer gelöscht",    "Timer deleted" },
  { "Vorlage gelöscht",  "Preset deleted" },
  { "+1 Minute",         "+1 minute" },
  { "+5 Minuten",        "+5 minutes" },
  { "Zeit einstellen",   "Set a time first" },
  { "Zu viele Timer",    "Too many timers" },
  { "Eieruhr läuft",     "Egg timer running" },
  { "Alarm scharf",      "Alarm armed" },
  { "Alarm aus",         "Alarm off" },
  { "Alarm aus",         "Alarm off" },
  { "Keine Haptik gefunden", "No haptics found" },
  // Rueckfragen
  { "Timer löschen?",    "Delete timer?" },
  { "Vorlage löschen?",  "Delete preset?" },
  // Zustaende
  { "abgelaufen",        "time is up" },
  { "pausiert",          "paused" },
  { "fertig",            "done" },
  { "Kein Timer läuft",  "No timer running" },
  { "suche ...",         "searching ..." },
  { "aus",               "off" },
  { "läuft im Hintergrund weiter", "keeps running in the background" },
  { "Zieltemperatur erreicht", "Target temperature reached" },
  { "Fühler aus der Ladeschale nehmen", "Take the probe out of the charger" },
  { "Noch keine Vorlagen.\nEin gestarteter Timer\nwird automatisch eine.",
    "No presets yet.\nAny timer you start\nbecomes one." },
  { "lang tippen = mehr", "long press = more" },
  { "Drehring stellt die Helligkeit", "The ring sets the brightness" },
  // Einstellungen
  { "Helligkeit",        "Brightness" },
  { "Lautstärke",        "Volume" },
  { "Sprache",           "Language" },
  { "Fühler-Akku",       "Probe battery" },
  // Eieruhr
  { "Grösse",            "Size" },
  { "Start",             "From" },
  { "Ergebnis",          "Result" },
  { "Kühlschrank",       "Fridge" },
  { "Zimmer",            "Room" },
  { "weich",             "soft" },
  { "wachsweich",        "medium" },
  { "hart",              "hard" },
  // Garstufen
  { "blutig",            "very rare" },
  { "englisch",          "rare" },
  { "rosa",              "medium rare" },
  { "medium",            "medium" },
  { "halb durch",        "medium well" },
  { "durch",             "well done" },
  { "sehr durch",        "very well" },
  // Thermometer
  { "Ziel",              "Target" },
  { "Garraum",           "Ambient" },
};

static_assert(sizeof(TXT) / sizeof(TXT[0]) == TXT_COUNT,
              "Textkatalog und TxtId sind aus dem Tritt geraten");

const char *T(TxtId id) { return TXT[id][s_lang]; }
Lang lang_get() { return s_lang; }
uint8_t lang_rev() { return s_rev; }
const char *lang_label(Lang l) { return l == LANG_EN ? "English" : "Deutsch"; }

const char *lang_btn(const char *symbol, TxtId id) {
  static char buf[4][48];
  static uint8_t n = 0;
  n = (uint8_t)((n + 1) % 4);
  snprintf(buf[n], sizeof(buf[0]), "%s%s", symbol, T(id));
  return buf[n];
}

void lang_set(Lang l) {
  if (l == s_lang) return;
  s_lang = l;
  s_rev++;          // laesst alle Zeichen-Wachen der Screens ausloesen
}
