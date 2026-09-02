#!/usr/bin/env python3
"""Wandelt ein PNG in ein LVGL-Farbbild (RGB565, wie LV_COLOR_DEPTH 16).

Der andere Konverter (convert_icons.py) erzeugt reine Alphakanaele zum
Einfaerben - fuer ein mehrfarbiges Logo braucht es echte Farbwerte.

    python3 tools/convert_image.py assets/logos/meater.png logo_meater 200
"""
import sys, os
from PIL import Image

src, name, width = sys.argv[1], sys.argv[2], int(sys.argv[3])
im = Image.open(src).convert("RGB")
h = round(im.height * width / im.width)
im = im.resize((width, h), Image.LANCZOS)

rows, data = [], bytearray()
for y in range(h):
    for x in range(width):
        r, g, b = im.getpixel((x, y))
        v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        data += bytes((v & 0xFF, v >> 8))          # little endian, LV_COLOR_16_SWAP 0
for i in range(0, len(data), 16):
    rows.append("  " + " ".join("0x%02x," % b for b in data[i:i + 16]))

c = [f"/* Automatisch erzeugt: python3 tools/convert_image.py {src} {name} {width} */",
     '#include "lvgl.h"', "",
     "#ifndef LV_ATTRIBUTE_MEM_ALIGN", "#define LV_ATTRIBUTE_MEM_ALIGN", "#endif", "",
     f"static const LV_ATTRIBUTE_MEM_ALIGN uint8_t {name}_map[] = {{"] + rows + ["};", "",
     f"const lv_img_dsc_t {name} = {{",
     "  .header.cf = LV_IMG_CF_TRUE_COLOR,", "  .header.always_zero = 0,",
     "  .header.reserved = 0,", f"  .header.w = {width},", f"  .header.h = {h},",
     f"  .data_size = {len(data)},", f"  .data = {name}_map,", "};", ""]
open(f"src/gen/{name}.c", "w").write("\n".join(c))
open(f"src/gen/{name}.h", "w").write(
    f'/* Automatisch erzeugt. */\n#pragma once\n#include "lvgl.h"\nLV_IMG_DECLARE({name});\n')
print(f"{name}: {width}x{h}, {len(data)//1024} KB Flash -> src/gen/{name}.c")
