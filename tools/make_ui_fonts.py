#!/usr/bin/env python3
"""Erzeugt die Oberflaechenschrift: Rubik mit Umlauten plus die LVGL-Symbole.

Warum ueberhaupt: Die eingebauten Montserrat-Schriften von LVGL enthalten nur
ASCII (`-r 0x20-0x7F,0xB0,0x2022`). Ein "ü" ist dort schlicht nicht vorhanden -
deshalb stand auf dem Geraet frueher "Groesse" und "Fuehler".

Rubik ist ohnehin schon die Schrift der grossen Ziffern; damit passt jetzt auch
die Typografie zusammen. Die Symbole (Play, Pause, Glocke ...) stecken in
derselben Datei, die LVGL fuer seine eingebauten Schriften benutzt - deshalb
wird FontAwesome mit denselben Codepunkten dazugemischt.

    python3 tools/make_ui_fonts.py
"""
import os, subprocess, sys

SIZES = [14, 16, 20, 28]
RUBIK = os.path.expanduser("~/Library/Fonts/Rubik-VariableFont_wght.ttf")
FA = "assets/fonts/FontAwesome5-Solid+Brands+Regular.woff"

# Zeichen: ASCII, Gradzeichen, Mittelpunkt, deutsche Umlauts, Aufzaehlungspunkt
TEXT_RANGES = ["0x20-0x7F", "0xB0", "0xB7", "0xC4", "0xD6", "0xDC",
               "0xE4", "0xF6", "0xFC", "0xDF", "0x2013", "0x2022"]

# Symbolliste 1:1 aus LVGLs eigener Schrifterzeugung uebernommen, damit
# LV_SYMBOL_* unveraendert weiterfunktioniert.
FA_RANGE = ("61441,61448,61451,61452,61452,61453,61457,61459,61461,61465,61468,"
            "61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,"
            "61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,"
            "61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,"
            "61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,"
            "62099,62212,62189,62810,63426,63650")

if not os.path.exists(RUBIK):
    sys.exit("Rubik nicht gefunden: " + RUBIK)

os.makedirs("src/gen", exist_ok=True)
decls = ['/* Automatisch erzeugt von tools/make_ui_fonts.py. */', "#pragma once",
         '#include "lvgl.h"', ""]
total = 0
for size in SIZES:
    out = f"src/gen/font_ui_{size}.c"
    cmd = ["npx", "--prefix", "tools", "lv_font_conv", "--font", RUBIK]
    for r in TEXT_RANGES:
        cmd += ["-r", r]
    cmd += ["--font", FA, "-r", FA_RANGE,
            "--size", str(size), "--bpp", "4", "--format", "lvgl",
            "--no-compress", "--lv-include", "lvgl.h", "-o", out]
    subprocess.run(cmd, check=True, capture_output=True)
    kb = os.path.getsize(out) // 1024
    total += kb
    decls.append(f"LV_FONT_DECLARE(font_ui_{size});")
    print(f"  font_ui_{size}: {kb} KB Quelltext")

open("src/gen/font_ui.h", "w").write("\n".join(decls) + "\n")
print(f"gesamt {total} KB Quelltext -> src/gen/font_ui_*.c")
