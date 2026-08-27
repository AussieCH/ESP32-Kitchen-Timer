#!/usr/bin/env python3
"""Wandelt die Icon-PNGs (weisse Strichzeichnung auf schwarz) in LVGL-Bilder um.

Ausgabe: src/gen/icons_data.c + .h im Format LV_IMG_CF_ALPHA_8BIT - also nur ein
Alphakanal. Die Farbe kommt zur Laufzeit aus dem img_recolor-Style, damit jedes
Icon in der Farbe seines Timers erscheint.

Aufruf:  python3 tools/convert_icons.py assets/icons
"""
import sys, os, glob
from PIL import Image

# Genau die Groessen, die die Traeger in der UI haben - LVGL kann Alpha-Bilder
# nicht skalieren (lv_img_set_zoom zeichnet sie gar nicht), also passend erzeugen.
SIZES = (64, 36)
NOISE = 14          # alles darunter ist Kompressionsrauschen im schwarzen Grund

def load(path):
    im = Image.open(path).convert('L')
    px = im.point(lambda v: 0 if v < NOISE else v)
    bbox = px.getbbox()
    if bbox:
        px = px.crop(bbox)
    return px

def fit(px, size):
    w, h = px.size
    s = size / max(w, h)
    px = px.resize((max(1, round(w * s)), max(1, round(h * s))), Image.LANCZOS)
    out = Image.new('L', (size, size), 0)
    out.paste(px, ((size - px.size[0]) // 2, (size - px.size[1]) // 2))
    return out

def main(src_dir):
    files = sorted(glob.glob(os.path.join(src_dir, '*.png')))
    if not files:
        sys.exit(f'keine PNGs in {src_dir}')
    os.makedirs('src/gen', exist_ok=True)

    c = ['/* Automatisch erzeugt von tools/convert_icons.py - nicht von Hand aendern. */',
         '#include "lvgl.h"', '#include "icons_data.h"', '',
         '#ifndef LV_ATTRIBUTE_MEM_ALIGN', '#define LV_ATTRIBUTE_MEM_ALIGN', '#endif', '']
    h = ['/* Automatisch erzeugt von tools/convert_icons.py. */', '#pragma once',
         '#include "lvgl.h"', '',
         f'#define ICON_IMG_COUNT {len(files)}', '',
         *[f'extern const lv_img_dsc_t *const ICON_IMG_{s}[ICON_IMG_COUNT];' for s in SIZES], '']

    names = []
    for i, f in enumerate(files):
        base = os.path.splitext(os.path.basename(f))[0]
        sym = 'icon_%02d_%s' % (i, ''.join(ch if ch.isalnum() else '_' for ch in base).lower())
        names.append(sym)
        src = load(f)
        for size in SIZES:
            img = fit(src, size)
            data = img.tobytes()
            rows = []
            for y in range(size):
                row = data[y * size:(y + 1) * size]
                rows.append('  ' + ' '.join('0x%02x,' % b for b in row))
            c += [f'static const LV_ATTRIBUTE_MEM_ALIGN uint8_t {sym}_{size}_map[] = {{']
            c += rows
            c += ['};', f'const lv_img_dsc_t {sym}_{size} = {{',
                  '  .header.cf = LV_IMG_CF_ALPHA_8BIT,', '  .header.always_zero = 0,',
                  '  .header.reserved = 0,', f'  .header.w = {size},', f'  .header.h = {size},',
                  f'  .data_size = {size * size},', f'  .data = {sym}_{size}_map,', '};', '']
            h.append(f'LV_IMG_DECLARE({sym}_{size});')

    h.append('')
    for size in SIZES:
        c.append(f'const lv_img_dsc_t *const ICON_IMG_{size}[ICON_IMG_COUNT] = {{')
        c += ['  &%s_%d,' % (n, size) for n in names]
        c += ['};', '']

    open('src/gen/icons_data.c', 'w').write('\n'.join(c))
    open('src/gen/icons_data.h', 'w').write('\n'.join(h))
    print(f'{len(files)} Icons -> src/gen/icons_data.c '
          f'({os.path.getsize("src/gen/icons_data.c")//1024} KB Quelltext, '
          f'{len(files) * sum(s*s for s in SIZES)//1024} KB im Flash)')
    for i, f in enumerate(files):
        print(' %2d  %s' % (i, os.path.basename(f)))

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'assets/icons')
