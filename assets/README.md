# Assets: Icons und Melodien

## Icons

**Geliefert und eingebaut.** 33 Stueck, weisse Strichzeichnung auf schwarz, in
`assets/icons/` als `<ID>_<Name>.png` (Originale, 1254x1254). Der Dateiname ist
die Quelle fuer Reihenfolge **und** Anzeigenamen.

Aus den PNGs erzeugt `tools/convert_icons.py` die LVGL-Bilddaten:

```bash
python3 tools/convert_icons.py assets/icons
```

Das schreibt `src/gen/icons_data.{c,h}` im Format **LV_IMG_CF_ALPHA_8BIT** - also
nur ein Alphakanal, die Helligkeit der Strichzeichnung wird zur Deckkraft. Die
Farbe kommt zur Laufzeit aus dem `img_recolor`-Style, damit jedes Icon in der
Farbe seines Timers erscheint. 33 Icons in zwei Groessen kosten rund 175 KB Flash.

Zwei Dinge, die man wissen muss:

* Erzeugt werden genau die Groessen, die die Traeger in der UI haben (**64** und
  **36** px). LVGL kann Alpha-Bilder **nicht** skalieren - `lv_img_set_zoom`
  zeichnet sie schlicht gar nicht. Neue Traegergroesse? Dann `SIZES` im Skript
  ergaenzen und in `icons.cpp` auswaehlen, nicht zoomen.
* Die **Reihenfolge ist die Icon-ID** und steckt in gespeicherten Vorlagen. Wird
  umsortiert, zeigen alte Vorlagen auf falsche Icons - dann `PRESET_VERSION` in
  `src/timers.cpp` hochzaehlen, damit das NVS beim naechsten Start verworfen wird.

Neues Icon im gleichen Stil: PNG mit weisser Strichzeichnung auf schwarzem
Grund, moeglichst flaechig (keine 1-px-Linien - das Panel ist rund, klein und
wird aus 2 m Entfernung abgelesen), nach `assets/icons/` legen, Skript laufen
lassen, `ICON_COUNT` in `src/icons.h` anpassen.

## Melodien

**Entfallen.** Das Board hat keinen Lautsprecher (der DAC gibt nur auf die
3.5-mm-Klinke aus), deshalb ist der Alarm optisch und haptisch. Die gelieferten
Notentabellen liegen nicht mehr im Projekt - falls der Ton je zurueckkommen
soll, waeren `Toene.txt` und `tools/u4wdh_xsmt/` der Ausgangspunkt.
