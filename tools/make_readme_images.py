#!/usr/bin/env python3
"""Erzeugt die Bilder fuer das README aus den Simulator-Screenshots.

Freigestellt als Kreise mit transparentem Hintergrund - so sitzen sie auf
GitHubs hellem wie dunklem Design sauber, ohne grauen Kasten drumherum.

Vorher beide Sprachlaeufe machen, sonst fehlt der englische Satz:
    ./tools/sim/build.sh && ./tools/sim/build.sh en
"""
from PIL import Image, ImageDraw
import os, sys

PAARE = [("0-startbild", "logo"), ("2-aktiv-drei", "aktiv"),
         ("2b-uebersicht", "uebersicht"), ("3-neuer-timer", "neuer-timer"),
         ("3c-eieruhr", "eieruhr"), ("3e-fuehler", "thermometer"),
         ("3g-fuehler-offline", "thermometer-offline"), ("6-alarm", "alarm")]
S = 480

def kreise(prefix, ziel):
    os.makedirs(ziel, exist_ok=True)
    for a, b in PAARE:
        # Der Simulator schreibt PPM; PNG entsteht erst hier
        quelle = os.path.join("tools/sim/out", prefix + a + ".ppm")
        if not os.path.exists(quelle):
            print(f"  fehlt: {quelle} - Simulator laufen lassen"); sys.exit(1)
        im = Image.open(quelle).convert("RGBA").resize((S, S), Image.LANCZOS)
        m = Image.new("L", (S, S), 0)
        ImageDraw.Draw(m).ellipse((0, 0, S - 1, S - 1), fill=255)
        im.putalpha(m)
        im.save(os.path.join(ziel, b + ".png"), optimize=True)
        print(" ", os.path.join(ziel, b + ".png"))

kreise("", "docs/img")           # deutsche Oberflaeche
kreise("en-", "docs/img/en")     # englische, fuer den englischen Teil des README
