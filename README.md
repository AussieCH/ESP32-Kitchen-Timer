# Kuechentimer  -  Waveshare ESP32-S3-Knob-Touch-LCD-1.8

Autarker Kuechentimer auf dem 1.8"-Runddisplay mit Drehring. Kein WLAN, kein
Home Assistant, keine RTC - alles laeuft lokal auf dem Geraet.
Arduino-Core 3.x (pioarduino) + LVGL 8.4 + LovyanGFX (`Panel_ST77916`, QSPI).

## Bedienung

Eine Regel, ueberall gleich:

* **horizontal wischen** = Screen wechseln
* **Drehring** = immer der Kontext im aktuellen Screen, nie Navigation
* **vertikal wischen** = innerhalb einer Liste blaettern

Der Knopf hat **keinen Taster** (das Board hat schlicht keinen) - jedes
Bestaetigen laeuft ueber Touch. Fuenf Screens, unten zeigen Punkte die Position;
nach dem Start steht man auf *Aktiv*:

| Screen | Inhalt | Drehring |
|---|---|---|
| **Uebersicht** (links von *Aktiv*) | kompakte Liste aller laufenden Timer, eine Zeile je Timer mit Restzeit und Zustand; **antippen holt ihn in den Aktiv-Screen** | scrollt die Liste |
| **Aktiv** | Restzeit gross, aussen ein Fortschrittsring in der Timer-Farbe, Buttons *Pause/Weiter* und *+1 Min*, oben ✕ (mit Rueckfrage) | wechselt zwischen laufenden Timern |
| **Neuer Timer** | `hh:mm:ss` (Segment antippen), Schnellwahl 3/5/10/15 Min, Melodie- und Icon-Button, *Start* + ✕ | stellt das gewaehlte Segment / die Melodie / das Icon |
| **Vorlagen** | Liste; **tippen = starten**, **lang tippen** = Starten/Editieren/Loeschen | scrollt die Liste |
| **Einstellungen** | Lautstaerke und Helligkeit (Zeile antippen = auswaehlen), Testton | stellt die gewaehlte Zeile |

**Alarm ist optisch und haptisch, ohne Ton.** Laeuft ein Timer ab, holt das
Geraet den Aktiv-Screen nach vorn, weckt das Display, blinkt im Sekundentakt in
der Farbe des Timers und laesst die Haptik im selben Takt schnarren (DRV2605 im
RTP-Modus). Die halbe Bildmitte ist dann Stopptaste - man tippt mit dem
Handruecken, nicht mit der Fingerspitze -, die beiden Buttons werden zu *Stopp*
und *+5 Min*. Nach 5 Minuten hoert das Blinken von selbst auf, der Timer bleibt
als *abgelaufen* stehen.

Warum kein Ton: **das Board hat weder Lautsprecher noch Verstaerker.** Der
PCM5100A gibt auf Line-Pegel an die 3.5-mm-Klinke - ein Kuechentimer, der nur
mit angesteckter Aktivbox piept, ist keiner. Die Tonausgabe ist deshalb
komplett entfernt (I2S, Melodien, Lautstaerke). Wer sie zurueckholen will:
`tools/u4wdh_xsmt/` bleibt liegen, denn der DAC laesst sich nur entstummen,
wenn auf der zweiten MCU IO32 dauerhaft HIGH liegt.

Weitere Festlegungen:

* Timer laufen ueber **absolute Zielzeitpunkte** (`esp_timer`), nicht ueber
  Herunterzaehlen pro Tick - sonst driftet die Anzeige, sobald das UI haengt.
* Max. **8 gleichzeitige** Timer, max. **20 Vorlagen** (NVS).
* Ein gestarteter Timer wird automatisch zur Vorlage (gleiche Zeit + gleiches
  Icon aktualisiert die vorhandene, statt sie zu duplizieren).
* Nach 60 s ohne Eingabe geht das Backlight aus; Timer laufen weiter, ein
  Ablauf weckt. Die Beruehrung bzw. Rastung, die aufweckt, loest nichts aus.
* Lautstaerke und Helligkeit liegen im NVS und ueberleben den Neustart.
  Laufende Timer ueberleben einen Reboot **nicht** - ohne RTC ginge das nur
  unsauber.

## Inbetriebnahme

**Firmware flashen.** Die zweite MCU wird nicht gebraucht (siehe Alarm oben).

```bash
pio run -t upload --upload-port /dev/cu.usbmodem31201
```

Der erste Flash **muss ueber USB** laufen (16 MB, eigene Partitionstabelle).

**Kontrolle.** `pio device monitor` sollte zeigen:

```
[touch] CST816 ok
[haptic] DRV2605 ok (LRA)
[knob] EC1 bereit, Ruhelage AB=3
```

Stimmen Rot und Blau nicht, `cfg.rgb_order` in `src/lgfx_knob.h` umdrehen; ist
alles invertiert, `cfg.invert`. Landen Beruehrungen woanders, die Schalter
`TP_SWAP_XY` / `TP_MIRROR_X` / `TP_MIRROR_Y` in `src/board_pins.h`.

## Drehring

Der Geber rastet bei A=B=1 und schnappt pro Rastung durch einen kompletten
Quadraturzyklus - so schnell, dass die ISR nur **eine** Zwischenstufe zu sehen
bekommt (gemessen: nur `3->2->3` und `3->1->3`, nie die 0). Eine klassische
Flankensumme hebt sich damit exakt auf und der Ring waere tot. Deshalb:

* die **erste Zwischenstufe nach dem Verlassen der Ruhelage** gibt die Richtung,
* gezaehlt wird bei der Rueckkehr in die Ruhelage,
* 12 ms Sperrzeit gegen Prellen (ein Klick erzeugt rund 12 Flanken),
* ein Gegenschritt innerhalb von 80 ms gilt als Nachprellen, nicht als
  Richtungswechsel (echte Wechsel liegen im Mitschnitt bei >= 100 ms),
* Beschleunigung erst nach 6 zuegigen Rastungen am Stueck (5er-Schritte) bzw. 14
  (10er) - frueher greift sie beim normalen Bedienen und die Anzeige springt.

Dreht der Ring gefuehlt verkehrt herum: `KNOB_INVERT` in `src/board_pins.h`.

## Layout im Simulator pruefen

Das runde Panel verzeiht keine Ecken: was ausserhalb des Kreises liegt, ist
weg. `tools/sim` uebersetzt **dieselben** UI-Dateien fuer macOS, rendert alle
Screens nach `tools/sim/out/*.png` und meldet jedes Widget, das aus dem Kreis
ragt.

```bash
./tools/sim/build.sh
```

## Assets

**32 Icons** (weisse Strichzeichnung, zur Laufzeit in der Timer-Farbe
eingefaerbt) liegen als Originale in `assets/icons/`; die LVGL-Daten erzeugt
`python3 tools/convert_icons.py assets/icons`. Details und Fallstricke:
`assets/README.md`.

## Aufbau

```
src/board_pins.h     Pinout (schaltplanverifiziert)
src/lgfx_knob.h      LovyanGFX-Config (ST77916 QSPI, Backlight 20 kHz)
src/input_touch.*    CST816-Poller (bewusst ohne Fremdbibliothek)
src/input_knob.*     Quadratur-Dekodierung EC1 + Drehbeschleunigung
src/haptics.*        DRV2605 als LRA: Rastung, Bestaetigung, Alarm-Schnarren
src/timers.*         Datenmodell, Ablauflogik, NVS
src/ui*.cpp          Rahmen, fuenf Screens, Alarmzustand im Aktiv-Screen
src/gen/             erzeugt: Ziffernfonts (Rubik 92/52/30) + Icon-Bilddaten
assets/icons/        32 Quell-PNGs, Dateiname = ID und Anzeigename
tools/sim/           Host-Simulator + Layoutpruefung
tools/u4wdh_xsmt/    Minimal-Sketch fuer den zweiten Chip (DAC entstummen)
```
