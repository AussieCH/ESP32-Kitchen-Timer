<p align="center">
  <img src="docs/img/logo.png" width="180" alt="RONDO">
</p>

<h1 align="center">RONDO</h1>

<p align="center">
  Autarker Küchentimer auf einem 1.8"-Runddisplay mit Drehring.<br>
  Kein WLAN, kein Konto, keine App — alles läuft lokal auf dem Gerät.
</p>

<p align="center">
  <img src="docs/img/aktiv.png" width="150" alt="Laufender Timer">
  <img src="docs/img/uebersicht.png" width="150" alt="Übersicht aller Timer">
  <img src="docs/img/thermometer.png" width="150" alt="Grill-Thermometer">
  <img src="docs/img/alarm.png" width="150" alt="Alarm">
</p>

<p align="center">
  <img src="docs/img/neuer-timer.png" width="150" alt="Neuer Timer">
  <img src="docs/img/eieruhr.png" width="150" alt="Eieruhr">
  <img src="docs/img/thermometer-offline.png" width="150" alt="Thermometer ohne Fühler">
</p>

<p align="center"><sub>Alle Abbildungen kommen aus dem Host-Simulator — das ist
wirklich das, was auf dem Gerät steht.</sub></p>

Gebaut mit Arduino-Core 3.x (pioarduino), LVGL 8.4 und LovyanGFX
(`Panel_ST77916`, QSPI). Eine ausführliche Bedienungsanleitung liegt als
[PDF](RONDO-Anleitung.pdf) bei.

## Hardware

**Guition JC3636K718C** ("K/Knob"-Serie): ESP32-S3, 8 MB Octal-PSRAM, 16 MB
Flash, rundes 360×360-Display (ST77916, QSPI), CST816-Touch, Drehknopf ohne
Taster, PCM5100A mit **eingebautem Lautsprecher**, **WS2812-Ring mit 13 LEDs**,
DRV2605-Haptik (LRA), Akku-ADC, PDM-Mikrofon, microSD.

Der Pinout steht in [`src/board_pins.h`](src/board_pins.h) — aus dem
Herstellerdemo `JC3636K718_knob_EN` und am Gerät nachgeprüft. Er unterscheidet
sich komplett vom JC3636W518; **Pins anderer Boards niemals hierher kopieren.**

Eine Eigenheit, die keine Quelle erwähnt: der **DRV2605 auf `0x5A` ist
bestückt**, obwohl weder das Herstellerdemo noch verbreitete Pinouts ihn
führen. Die Firmware scannt deshalb beim Start den I²C-Bus und schreibt ins
Log, was sie findet.

## Bedienung

Eine Regel, überall gleich:

* **horizontal wischen** = Screen wechseln
* **Drehring** = immer der Kontext im aktuellen Screen, nie Navigation
* **vertikal wischen** = innerhalb einer Liste blättern

Der Knopf hat **keinen Taster** — jedes Bestätigen läuft über Touch. Acht
Screens, unten zeigen Punkte die Position; nach dem Start steht man auf *Aktiv*:

| Screen | Inhalt | Drehring |
|---|---|---|
| **Übersicht** | kompakte Liste aller laufenden Timer (und der Stoppuhr) mit Zeit und Zustand; **antippen holt einen in den Aktiv-Screen** | scrollt die Liste |
| **Aktiv** | Restzeit groß, außen ein Fortschrittsring in der Timer-Farbe, Buttons *Pause/Weiter* und *+1 Min*, oben ✕ (mit Rückfrage). Eine laufende **Stoppuhr** ist hier ein Eintrag wie ein Timer. Läuft nichts: das RONDO-Zeichen mit langsam wanderndem Punkt, Tippen führt zum neuen Timer | wechselt zwischen laufenden Timern und der Stoppuhr |
| **Neuer Timer** | `hh:mm:ss` (Segment antippen), Schnellwahl 3/5/10/15 Min, Melodie- und Icon-Button, *Start* + ✕ | stellt Segment, Melodie oder Icon |
| **Eieruhr** | Größe, Kühlschrank/Zimmer, weich/wachsweich/hart → rechnet die Kochzeit und startet den Timer | stellt die gewählte Zeile |
| **Grill-Thermometer** | Kerntemperatur des MEATER-Fühlers gross, dazu Garraum, Fühler-Akku und Verbindungszustand; **Zieltemperatur mit Alarm** | stellt die Zieltemperatur |
| **Stoppuhr** | zählt hoch, mit Zehnteln; läuft im Hintergrund weiter und erscheint dann auch im Aktiv-Screen und in der Übersicht | — |
| **Vorlagen** | Liste; **tippen = starten**, **lang tippen** = Starten/Editieren/Löschen | scrollt die Liste |
| **Einstellungen** | Helligkeit, Lautstärke, *Alarm testen*, Akkuzustand | stellt die gewählte Zeile |

Beim Start läuft das **RONDO-Logo**: 13 Punkte im Kreis, der orange wandert
einmal herum — synchron über die 13 echten LEDs des Rings. Antippen überspringt.
Gezeichnet statt eingebettet: ein paar hundert Byte statt 260 KB, und bei jeder
Größe scharf.

### Alarm

Läuft ein Timer ab, holt das Gerät den **Aktiv-Screen nach vorn**, weckt das
Display und blinkt im 130-ms-Takt in der Farbe des Timers. Im selben Takt
schnarrt die Haptik (DRV2605 im RTP-Modus), blinkt der LED-Ring und spielt die
gewählte Melodie in Schleife mit 8-Sekunden-Lautstärkerampe.

Die halbe Bildmitte ist dann Stopptaste — man tippt mit dem Handrücken, nicht
mit der Fingerspitze —, die beiden Buttons werden zu *Stopp* und *+5 Min*. Nach
5 Minuten hört der Alarm von selbst auf, der Timer bleibt als *abgelaufen*
stehen.

### LED-Ring

Der Ring zeigt den Timer, der gerade im Aktiv-Screen steht — nicht immer den
ersten. Schaltet man mit dem Drehring weiter, wechselt er mit.

**Die Farbe ist die Identität des Timers**, nicht sein Fortschritt: jeder
laufende Timer bekommt eine eigene aus einer Palette von acht (vergeben wird
die erste, die kein anderer laufender Timer hat), und dieselbe Farbe färbt
Icon, Fortschrittsbogen und LED-Ring. Der Ring füllt sich anteilig mit der
Restzeit; in den letzten zehn Sekunden pulsiert er.

> Ein früherer Entwurf ließ den Ring von grün über amber nach rot laufen. Das
> beißt sich mit der Timer-Identität — eine Farbe kann nicht gleichzeitig
> „welcher Timer" und „wie viel Restzeit" bedeuten. Geblieben ist das
> Pulsieren am Ende, das signalisiert Dringlichkeit ohne die Farbe zu belegen.

Läuft die **Stoppuhr** und ist sie ausgewählt, wandert stattdessen ein einzelner
cremefarbener Punkt im Sekundentakt herum — sie hat keine Restzeit, die man
füllen könnte, aber „läuft" liest man von weitem.

### Grill-Thermometer (MEATER)

Verifiziert mit einem **MEATER Plus** (Fühler `MT-PR10`, Ladeschale `MT-CP01`).
Der Fühler funkt Bluetooth Low Energy — genau das, was der ESP32-S3 kann.

Verbunden wird mit der **Ladeschale**, nicht mit dem Fühler direkt: sie ist der
Verstärker, damit ist die Reichweite kein Thema. Es braucht **keine Kopplung**,
der Fühler sendet von sich aus (`notify`). Er meldet sich allerdings nur,
solange er **aus der Ladeschale genommen** ist.

| | |
|---|---|
| Dienst | `a75cc7fc-c956-488f-ac2a-2dbc08b63a04` |
| Temperatur | `7edda774-045e-4bbf-909b-45d1991a2876`, 8 Bytes, notify |
| Akku | `2adb4877-68d8-4884-bd3c-d83853bf27b8` |
| Kerntemperatur | `(x[0] + (x[1]<<8) + 8) / 16` = °C |
| Garraum | `(tip + max(0, ((ra - min(48, oa)) * 16 * 589) / 1487) + 8) / 16` |

Ohne Verbindung zeigt der Screen das **MEATER-Logo** statt Platzhalterstrichen —
„--.-" sah nach Fehler aus, das Logo sagt, worauf das Gerät wartet.

**Zieltemperatur.** Der Drehring stellt sie (30–99 °C), daneben steht die
Garstufe in Worten — man denkt eher in „rosa" als in 58 °C. Ist der Alarm
scharf und wird das Ziel erreicht, läuft **derselbe Alarm wie bei einem
abgelaufenen Timer**. Deshalb liegt die Alarmwirkung in `src/alarm.*` und nicht
mehr im Timer-Screen: Blinken, Haptik, Melodie und LED-Ring haben zwei Auslöser,
und zwei Kopien davon wären eine zu viel. Ziel und Scharfschaltung liegen im NVS.

Kennungen und Umrechnung stammen aus der offengelegten Arbeit der Community
([meaterble](https://github.com/nathanfaber/meaterble), [ESPHome-Gist](https://gist.github.com/MortenVinding/a513c0094d0df41a4425612257b3cabc))
und wurden am Gerät gegengeprüft. Beim Verbinden schreibt die Firmware **alle
gefundenen Dienste und Merkmale ins Log** — das war kein Debug-Überbleibsel,
sondern der Beweis, dass dieses Exemplar die dokumentierten Kennungen benutzt.
Es bleibt drin: neuere Modelle (MEATER 2 Plus, SE) verwenden andere Kennungen,
und dann sieht man das in zwei Minuten statt es zu raten.

**Preis in Speicher:** NimBLE nimmt sich rund **100 KB internen RAM**. Danach
waren nur noch 22 KB frei — zu wenig, das wäre früher oder später abgestürzt.
Deshalb läuft der LVGL-Pool jetzt mit 96 statt 128 KB (Spitzenverbrauch laut
Simulator: 64 KB); damit sind wieder rund 55 KB frei.

NimBLE selbst läuft dabei in der **Standardkonfiguration**, also mit bis zu drei
gleichzeitigen Verbindungen — die gemessenen 55 KB gelten für diesen Zustand.

Zur Einordnung der Speicherzahlen, weil sie leicht durcheinandergehen: Der S3
hat **512 KB SRAM**, davon stehen dem Linker **320 KB** für statische Daten zu
(der Rest ist IRAM, Cache und ROM-Reservierung), und der Allokator verwaltet zur
Laufzeit **234 KB Heap**. Frei sind davon rund **53 KB**, der grösste
zusammenhängende Block misst **30 KB** — letzteres ist die Grenze, gegen die
eine einzelne grosse Allokation zuerst läuft. Die 8 MB PSRAM sind praktisch
unberührt; LVGLs Pool liegt bewusst im schnellen internen RAM, liesse sich aber
dorthin verschieben, falls es je eng wird.
Für mehrere Fühler (MEATER Duo, Block) ist der Stack damit bereits gerüstet;
begrenzt ist nur die Anwendung, die heute genau einen Fühler führt.

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
* Die **Akkuanzeige ist eine Heuristik**: über 4.25 V (Laden) wird bewusst nur
  die Spannung gezeigt, keine erfundene Prozentzahl. Die Kennlinie gehört pro
  Gerät nachjustiert.
* Ändert sich das Datenformat der Vorlagen oder die Icon-Reihenfolge, muss
  `PRESET_VERSION` in `src/timers.cpp` hoch — sonst zeigen alte Vorlagen auf
  falsche Icons.
* **RONDO übernimmt den Fühler.** Ein MEATER-Fühler lässt genau eine Verbindung
  zu, und die gehört hier dem Gerät — die Hersteller-App bleibt aussen vor. Das
  ist Absicht: RONDO ist für jemanden gebaut, der seine Temperaturen kennt und
  keinen Assistenten braucht, sondern eine Zahl und einen Alarm. Verbunden wird
  mit der Ladeschale (dem Verstärker), nicht mit dem Fühler direkt.
* **Zeichenpuffer: 2 × 20 Bildzeilen** statt 40. Das macht 29 KB internen RAM
  frei (53 → 82 KB frei, grösster zusammenhängender Block 30 → 41 KB) und kostet
  gemessen ein Drittel mehr Zeit je Vollbild (42 → 56 ms, weil LVGL 16 statt 10
  Übertragungen braucht). Am Gerät ist der Unterschied nicht wahrnehmbar; die
  Messung lässt sich mit `-D PERF_TEST` jederzeit wiederholen.
* **LVGL-Heap: 96 KB.** Mit 64 KB schlug `lv_mem_alloc` beim Aufbau der
  Alarmmasken fehl, und LVGLs Assert-Handler ist ein `while(1)` — das Gerät wäre
  im Alarm stillschweigend stehengeblieben. Der Simulator schreibt zu jedem
  Screen den freien Heap mit, damit so etwas auffällt.

## Inbetriebnahme

```bash
pio run -t upload --upload-port /dev/cu.usbmodemXXXX
```

Der erste Flash **muss über USB** laufen (16 MB, eigene Partitionstabelle).

Ohne Zutun taucht kein serieller Port auf — die Werksfirmware meldet sich als
`ESP USB DEVICE` (PID `0x4001`). Für den Download-Modus: **Strom ganz weg**,
Büroklammer ins **BOOT-Pinhole links neben USB-C**, drücken und halten, *dann*
USB einstecken, ~2 s halten, loslassen. Danach erscheint `cu.usbmodem*`.

Nach dem Flashen hilft **kein** esptool-Reset — der bootet immer wieder in den
Download-Modus (`rst:0x15`); es braucht einen echten Power-On-Reset, also USB
abziehen und neu einstecken.

**Kontrolle.** `pio device monitor` sollte zeigen:

```
[boot]  Kuechentimer auf Guition JC3636K718C
[touch] CST816 ok
[i2c]   gefunden: 0x15 0x5A
[haptic] DRV2605 ok (LRA)
[knob]  Impulsgeber bereit (links GPIO2, rechts GPIO1)
[audio] I2S ok
[batt]  4.33 V (Zelle)
```

Stimmen Rot und Blau nicht, `cfg.rgb_order` in `src/lgfx_knob.h` umdrehen; ist
alles invertiert, `cfg.invert`. Landen Berührungen woanders, die Schalter
`TP_SWAP_XY` / `TP_MIRROR_X` / `TP_MIRROR_Y` in `src/board_pins.h`.

## Der Drehknopf

Das ist **kein Quadraturgeber** — ein PCNT-Encoder liest hier null. Pro Rastung
kommt ein LOW-Impuls, links auf GPIO2, rechts auf GPIO1. Am Gerät
mitgeschnitten kamen zwei Eigenheiten dazu, ohne die sich der Ring zäh und
ungenau anfühlt:

* Es sind **zwei** Impulse pro Rastung (67–125 ms auseinander, der erste auf
  halbem Weg) — ungeteilt springt die Anzeige schon beim halben Klick.
  Deshalb `KNOB_PULSES_PER_DETENT` = 2.
* Bei jeder Rastung **klingelt die Gegenleitung nach**: ein Schwall in
  derselben Millisekunde plus einzelne Nachzügler noch nach ~50 ms. Die zählten
  als Gegenschritt und verwarfen die angefangene Rastung. Deshalb 4 ms Sperre
  je Leitung, und ein Richtungswechsel wird erst 150 ms nach dem letzten
  gezählten Impuls akzeptiert.

Beschleunigung greift erst nach 6 zügigen Rastungen am Stück (5er-Schritte)
bzw. 14 (10er) — früher greift sie beim normalen Bedienen und die Anzeige
springt. Dreht der Ring gefühlt verkehrt herum: `KNOB_INVERT`.

Zum Nachmessen `-D DIAG_MODE` in `platformio.ini` einkommentieren: grüne
Statuszeile auf dem Display plus Impulsprotokoll auf Serial (blockiert nicht,
auch ohne Zuhörer).

## Layout im Simulator prüfen

Das runde Panel verzeiht keine Ecken: was außerhalb des Kreises liegt, ist weg.
`tools/sim` übersetzt **dieselben** UI-Dateien für macOS, rendert alle Screens
nach `tools/sim/out/*.png`, meldet jedes Widget, das aus dem sichtbaren Kreis
ragt, und schreibt den freien LVGL-Heap mit.

```bash
./tools/sim/build.sh
```

## Assets

**33 Icons** (weiße Strichzeichnung, zur Laufzeit in der Timer-Farbe
eingefärbt) liegen als Originale in `assets/icons/`; die LVGL-Daten erzeugt
`python3 tools/convert_icons.py assets/icons`. **20 Melodien** als
`{Frequenz, Dauer}`-Tabellen in `src/melodies.cpp`. Details und Fallstricke:
[`assets/README.md`](assets/README.md).

## Aufbau

```
src/board_pins.h     Pinout (Herstellerdemo + am Geraet geprueft)
src/lgfx_knob.h      LovyanGFX-Config (ST77916 QSPI, Backlight 20 kHz)
src/input_touch.*    CST816-Poller (bewusst ohne Fremdbibliothek)
src/input_knob.*     Impulsdekodierung mit Teiler und Gegenleitungssperre
src/haptics.*        DRV2605 als LRA: Rastung, Bestaetigung, Alarm-Schnarren
src/audio.*          I2S-Tonsynthese, Lautstaerke, Fade-in
src/melodies.*       20 Alarmmelodien als Notentabellen
src/leds.*           WS2812-Ring: Restzeit in der Timer-Farbe, Alarmblinken, Logo
src/battery.*        Spannungsmessung mit Ueberabtastung und Kennlinie
src/meater.*         BLE-Client fuer das MEATER-Grill-Thermometer
src/alarm.*          Alarmwirkung (Blinken, Haptik, Melodie, Ring) fuer beide Ausloeser
assets/logos/        Quellbild fuers MEATER-Logo (tools/convert_image.py)
src/timers.*         Datenmodell, Ablauflogik, NVS
src/ui_splash.cpp    RONDO-Startbild
src/ui*.cpp          Rahmen, sieben Screens, Alarmzustand im Aktiv-Screen
src/gen/             erzeugt: Ziffernfonts (Rubik 92/52/30) + Icon-Bilddaten
assets/icons/        33 Quell-PNGs, Dateiname = ID und Anzeigename
tools/sim/           Host-Simulator + Layout- und Heappruefung
```

---

<sub>MEATER ist eine Marke von Apption Labs. Dieses Projekt steht in keiner
Verbindung zum Hersteller; die Anbindung nutzt das von der Community
offengelegte Bluetooth-Protokoll.</sub>
