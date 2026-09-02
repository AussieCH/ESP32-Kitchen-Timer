#!/bin/bash
# Baut die UI fuer macOS und rendert Screenshots nach tools/sim/out/.
set -e
cd "$(dirname "$0")/../.."
LV=.pio/libdeps/knob18/lvgl

INC="-I include -I src -I tools/sim/shims -I $LV -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE"

if [ ! -f tools/sim/obj/lvgl.a ]; then
  echo "LVGL einmalig uebersetzen ..."
  export INC
  find $LV/src -name '*.c' -print0 | xargs -0 -P 8 -n 1 sh -c \
    'clang -O1 -w $INC -c "$0" -o "tools/sim/obj/$(echo "$0" | md5 -q).o"'
  ar rcs tools/sim/obj/lvgl.a tools/sim/obj/*.o
fi

# Die Font-Dateien MUESSEN als C uebersetzt werden: in C++ haette
# "const lv_font_t x = {...}" interne Bindung und der Linker faende sie nicht.
for f in src/gen/*.c; do clang -x c -O1 -w $INC -c "$f" -o "tools/sim/obj/gen_$(basename $f).o"; done

clang++ -std=gnu++17 -O1 -w $INC \
  src/ui.cpp src/ui_splash.cpp src/ui_overview.cpp src/ui_active.cpp src/ui_egg.cpp src/ui_meater.cpp src/ui_stopwatch.cpp src/ui_new.cpp src/ui_presets.cpp src/ui_settings.cpp \
  src/icons.cpp src/timers.cpp src/melodies.cpp tools/sim/obj/gen_*.o \
  tools/sim/stubs.cpp tools/sim/sim_main.cpp \
  tools/sim/obj/lvgl.a -o tools/sim/sim
tools/sim/sim
