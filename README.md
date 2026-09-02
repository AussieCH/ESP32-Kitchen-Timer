# Küchentimer für runde ESP32-S3-Knob-Displays

Autarker Küchentimer auf einem 1.8"-Runddisplay mit Drehring. Kein WLAN, kein
Home Assistant, keine RTC — alles läuft lokal auf dem Gerät.
Arduino-Core 3.x (pioarduino) + LVGL 8.4 + LovyanGFX (`Panel_ST77916`, QSPI).

## Unterstützte Boards

Ein Quellbaum, zwei Ziele. Beide haben denselben Displaycontroller und denselben
Touch — sonst ist so gut wie nichts gleich, **Pins niemals zwischen den Boards
kopieren**.

| | **Waveshare ESP32-S3-Knob-Touch-LCD-1.8** | **Guition JC3636K718C** |
|---|---|---|
| Build | `pio run -e knob18` | `pio run -e guition` |
| Drehring | echte Quadratur (EC1, GPIO8/7) | **keine** Quadratur: je ein Impuls, links GPIO2 / rechts GPIO1 |
| Ton | **nein** — DAC nur auf die 3.5-mm-Klinke | **ja** — PCM5100A + eingebauter Lautsprecher |
| Haptik | DRV2605 (LRA) | DRV2605 (LRA), auf `0x5A` nachgemessen |
| LED-Ring | nein | **WS2812, 13 LEDs** |
| Zweite MCU | ja (ESP32-U4WDH, ungenutzt) | nein |

Die Unterschiede stehen in [`src/boards/`](src/boards) — Pins **und** Fähigkeiten
(`HAS_HAPTIC`, `HAS_AUDIO`, `HAS_LEDRING`, `KNOB_QUADRATURE`). Der übrige Code
fragt nur diese Flags ab; deshalb hat der Guition ohne Sonderfall die
Melodieauswahl und die Lautstärkezeile, der Waveshare nicht.

## Bedienung

Eine Regel, überall gleich:

* **horizontal wischen** = Screen wechseln
* **Drehring** = immer der Kontext im aktuellen Screen, nie Navigation
* **vertikal wischen** = innerhalb einer Liste blättern

Der Knopf hat auf beiden Boards **keinen Taster** — jedes Bestätigen läuft über
Touch. Fünf Screens, unten zeigen Punkte die Position; nach dem Start steht man
auf *Aktiv*:

| Screen | Inhalt | Drehring |
|---|---|---|
| **Übersicht** (links von *Aktiv*) | kompakte Liste aller laufenden Timer, eine Zeile je Timer mit Restzeit und Zustand; **antippen holt ihn in den Aktiv-Screen** | scrollt die Liste |
| **Aktiv** | Restzeit groß, außen ein Fortschrittsring in der Timer-Farbe, Buttons *Pause/Weiter* und *+1 Min*, oben ✕ (mit Rückfrage) | wechselt zwischen laufenden Timern |
| **Neuer Timer** | `hh:mm:ss` (Segment antippen), Schnellwahl 3/5/10/15 Min, Icon-Button (mit Ton zusätzlich Melodie), *Start* + ✕ | stellt das gewählte Segment / Icon / die Melodie |
| **Vorlagen** | Liste; **tippen = starten**, **lang tippen** = Starten/Editieren/Löschen | scrollt die Liste |
| **Einstellungen** | Helligkeit (mit Ton zusätzlich Lautstärke), *Alarm testen* | stellt die gewählte Zeile |

### Alarm

Läuft ein Timer ab, holt das Gerät den **Aktiv-Screen nach vorn**, weckt das
Display und blinkt im 130-ms-Takt in der Farbe des Timers. Im selben Takt läuft
mit, was das Board kann: die Haptik schnarrt (DRV2605 im RTP-Modus), der
LED-Ring blinkt, und auf Boards mit Lautsprecher spielt die gewählte Melodie in
Schleife mit 8-Sekunden-Lautstärkerampe.

Die halbe Bildmitte ist dann Stopptaste — man tippt mit dem Handrücken, nicht
mit der Fingerspitze —, die beiden Buttons werden zu *Stopp* und *+5 Min*. Nach
5 Minuten hört der Alarm von selbst auf, der Timer bleibt als *abgelaufen*
stehen.

Auf dem **Waveshare** gibt es bewusst keinen Ton: das Board hat weder
Lautsprecher noch Verstärker, der PCM5100A gibt auf Line-Pegel an die
3.5-mm-Klinke. Ein Küchentimer, der nur mit angesteckter Aktivbox piept, ist
keiner. Wer ihn dort trotzdem will, braucht `tools/u4wdh_xsmt/` — der DAC lässt
sich nur entstummen, wenn auf der **zweiten** MCU IO32 dauerhaft HIGH liegt.

Auf dem **Guition** zeigt der LED-Ring außerdem im Betrieb die Restzeit des
nächsten Timers als leuchtenden Bogen. Das sieht man quer durch die Küche, auch
wenn das Display längst dunkel ist.

### Weitere Festlegungen

* Timer laufen über **absolute Zielzeitpunkte** (`esp_timer`), nicht über
  Herunterzählen pro Tick — sonst driftet die Anzeige, sobald das UI hängt.
* Max. **8 gleichzeitige** Timer, max. **20 Vorlagen** (NVS).
* Ein gestarteter Timer wird automatisch zur Vorlage (gleiche Zeit + gleiches
  Icon aktualisiert die vorhandene, statt sie zu duplizieren).
* Nach 60 s ohne Eingabe geht das Backlight aus; Timer laufen weiter, ein Ablauf
  weckt. Die Berührung bzw. Rastung, die aufweckt, löst nichts aus.
* Helligkeit und Lautstärke liegen im NVS und überleben den Neustart. Laufende
  Timer überleben einen Reboot **nicht** — ohne RTC ginge das nur unsauber.
* Ändert sich das Datenformat der Vorlagen (oder die Icon-Reihenfolge), muss
  `PRESET_VERSION` in `src/timers.cpp` hoch — sonst zeigen alte Vorlagen auf
  falsche Icons.

## Inbetriebnahme

Der erste Flash **muss über USB** laufen (16 MB, eigene Partitionstabelle).

```bash
pio run -e knob18  -t upload --upload-port /dev/cu.usbmodemXXXX   # Waveshare
pio run -e guition -t upload --upload-port /dev/cu.usbmodemXXXX   # Guition
```

**Waveshare:** der USB-C-Port ist auf **beide** MCUs gemuxt. Klappt der Upload
nicht oder meldet sich das Board als `cu.usbserial-*` statt `cu.usbmodem*`,
hängt der ESP32-U4WDH dran — USB-A-auf-C-Kabel um 180° drehen. Bei C-auf-C
entscheidet der Host-Mux, also nicht deterministisch.

**Guition:** ohne Zutun taucht kein serieller Port auf, die Werksfirmware meldet
sich als `ESP USB DEVICE` (PID `0x4001`). Für den Download-Modus: **Strom ganz
weg**, Büroklammer ins **BOOT-Pinhole links neben USB-C**, drücken und halten,
*dann* USB einstecken, ~2 s halten, loslassen. Danach erscheint
`cu.usbmodem*`. Nach dem Flashen hilft **kein** esptool-Reset — der bootet
immer wieder in den Download-Modus (`rst:0x15`); es braucht einen echten
Power-On-Reset, also USB abziehen und neu einstecken.

**Kontrolle.** `pio device monitor` sollte zeigen (hier Guition):

```
[boot]  Kuechentimer auf Guition JC3636K718C
[touch] CST816 ok
[i2c]   gefunden: 0x15 0x5A
[haptic] DRV2605 ok (LRA)
[knob]  Impulsgeber bereit (links GPIO2, rechts GPIO1)
[audio] I2S ok
```

Die `[i2c]`-Zeile ist Absicht: beim Guition führt **keine** Quelle den DRV2605 —
weder das Herstellerdemo noch verbreitete Pinouts. Bestückt ist er trotzdem.

Stimmen Rot und Blau nicht, `cfg.rgb_order` in `src/lgfx_knob.h` umdrehen; ist
alles invertiert, `cfg.invert`. Landen Berührungen woanders, die Schalter
`TP_SWAP_XY` / `TP_MIRROR_X` / `TP_MIRROR_Y` im Board-Header.

## Die zwei Drehringe

Beide Geber sind eigenwillig, und beide auf völlig andere Art. Beides wurde am
Gerät **mitgeschnitten**, nicht angenommen — geraten wäre in beiden Fällen
falsch gewesen.

**Waveshare (Quadratur).** Der Geber rastet bei A=B=1 und schnappt pro Rastung
durch einen kompletten Quadraturzyklus — so schnell, dass die ISR nur **eine**
Zwischenstufe zu sehen bekommt (gemessen: nur `3→2→3` und `3→1→3`, nie die 0).
Eine Flankensumme hebt sich damit exakt auf, der Ring wäre tot. Deshalb: die
erste Zwischenstufe nach dem Verlassen der Ruhelage gibt die Richtung, gezählt
wird bei der Rückkehr in die Ruhelage, 12 ms Sperrzeit gegen Prellen (ein Klick
erzeugt rund 12 Flanken), und ein Gegenschritt innerhalb von 80 ms gilt als
Nachprellen.

**Guition (Impulse).** Keine Quadratur — ein PCNT-Encoder liest dort null.
Gemessen: **zwei** Impulse pro Rastung (67–125 ms auseinander, der erste auf
halbem Weg), und bei jeder Rastung **klingelt die Gegenleitung nach** — ein
Schwall in derselben Millisekunde plus einzelne Nachzügler noch nach ~50 ms.
Ungefiltert zählen die als Gegenschritt und verwerfen die angefangene Rastung;
das fühlt sich als Haken und Ungenauigkeit an. Deshalb: 4 ms Sperre je Leitung,
Teiler `KNOB_PULSES_PER_DETENT` = 2, und ein Richtungswechsel wird erst 150 ms
nach dem letzten gezählten Impuls akzeptiert.

Gemeinsam: Beschleunigung erst nach 6 zügigen Rastungen am Stück (5er-Schritte)
bzw. 14 (10er) — früher greift sie beim normalen Bedienen und die Anzeige
springt. Dreht der Ring gefühlt verkehrt herum: `KNOB_INVERT` im Board-Header.

Zum Nachmessen `-D DIAG_MODE` in `platformio.ini` anhängen: grüne Statuszeile
auf dem Display plus Flanken- bzw. Impulsprotokoll auf Serial (blockiert nicht,
auch ohne Zuhörer).

## Layout im Simulator prüfen

Das runde Panel verzeiht keine Ecken: was außerhalb des Kreises liegt, ist weg.
`tools/sim` übersetzt **dieselben** UI-Dateien für macOS, rendert alle Screens
nach `tools/sim/out/*.png` und meldet jedes Widget, das aus dem sichtbaren
Kreis ragt.

```bash
./tools/sim/build.sh            # Waveshare-Variante
./tools/sim/build.sh guition    # Guition-Variante (Screens mit g- davor)
```

## Assets

**33 Icons** (weiße Strichzeichnung, zur Laufzeit in der Timer-Farbe
eingefärbt) liegen als Originale in `assets/icons/`; die LVGL-Daten erzeugt
`python3 tools/convert_icons.py assets/icons`. **20 Melodien** als
`{Frequenz, Dauer}`-Tabellen in `src/melodies.cpp` (nur auf Boards mit
Lautsprecher hörbar). Details und Fallstricke: [`assets/README.md`](assets/README.md).

## Aufbau

```
src/board_pins.h     waehlt den Board-Header
src/boards/*.h       Pins + Faehigkeiten je Board
src/lgfx_knob.h      LovyanGFX-Config (ST77916 QSPI, Backlight 20 kHz)
src/input_touch.*    CST816-Poller (bewusst ohne Fremdbibliothek)
src/input_knob.*     beide Geber-Varianten hinter einer API
src/haptics.*        DRV2605 als LRA: Rastung, Bestaetigung, Alarm-Schnarren
src/audio.*          I2S-Tonsynthese (nur mit Lautsprecher)
src/melodies.*       20 Alarmmelodien als Notentabellen
src/leds.*           WS2812-Ring: Restzeit als Bogen, Alarmblinken
src/timers.*         Datenmodell, Ablauflogik, NVS
src/ui*.cpp          Rahmen, fuenf Screens, Alarmzustand im Aktiv-Screen
src/gen/             erzeugt: Ziffernfonts (Rubik 92/52/30) + Icon-Bilddaten
assets/icons/        33 Quell-PNGs, Dateiname = ID und Anzeigename
tools/sim/           Host-Simulator + Layoutpruefung
tools/u4wdh_xsmt/    Minimal-Sketch fuer die zweite MCU des Waveshare (DAC entstummen)
```
