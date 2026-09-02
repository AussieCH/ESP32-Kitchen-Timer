#!/usr/bin/env python3
"""Erzeugt die Bilder fuer das README aus den Simulator-Screenshots.

Freigestellt als Kreise mit transparentem Hintergrund - so sitzen sie auf
GitHubs hellem wie dunklem Design sauber, ohne grauen Kasten drumherum.
Vorher ./tools/sim/build.sh laufen lassen.
"""
from PIL import Image, ImageDraw
import os

PAARE = [("0-startbild.png", "logo.png"), ("2-aktiv-drei.png", "aktiv.png"),
         ("2b-uebersicht.png", "uebersicht.png"), ("3-neuer-timer.png", "neuer-timer.png"),
         ("3c-eieruhr.png", "eieruhr.png"), ("3e-fuehler.png", "thermometer.png"),
         ("3g-fuehler-offline.png", "thermometer-offline.png"), ("6-alarm.png", "alarm.png")]
S = 480
os.makedirs("docs/img", exist_ok=True)
for a, b in PAARE:
    im = Image.open(os.path.join("tools/sim/out", a)).convert("RGBA").resize((S, S), Image.LANCZOS)
    m = Image.new("L", (S, S), 0)
    ImageDraw.Draw(m).ellipse((0, 0, S - 1, S - 1), fill=255)
    im.putalpha(m)
    im.save(os.path.join("docs/img", b), optimize=True)
    print(" ", b)
