// Icon-Registry mit 32 Plaetzen.
//
// Die Grafiken liegen als reine Alphakanaele vor (weisse Strichzeichnung ->
// Deckkraft); eingefaerbt wird zur Laufzeit in der Farbe des jeweiligen Timers.
// Neu erzeugen: python3 tools/convert_icons.py assets/icons
// Reihenfolge = Icon-ID und steckt in gespeicherten Vorlagen -> nicht umsortieren.
#include "icons.h"
#include "gen/icons_data.h"

struct IconDef { const char *name; const char *abbrev; };

static const IconDef ICONS[ICON_COUNT] = {
  { "Pasta", "PA" },  { "Ei", "EI" },
  { "Hefezopf", "HE" },  { "Reis", "RE" },
  { "Kartoffeln", "KA" },  { "Gemuese", "GE" },
  { "Brokkoli", "BR" },  { "Spargel", "SP" },
  { "Suppe", "SU" },  { "Kochtopf", "KO" },
  { "Kochen", "KO" },  { "Braten", "BR" },
  { "Bratpfanne", "BR" },  { "Teig", "TE" },
  { "Gehen lassen", "GL" },  { "Brot", "BR" },
  { "Kuchen", "KU" },  { "Kekse", "KE" },
  { "Eintopf", "EI" },  { "Steak", "ST" },
  { "Filet", "FI" },  { "Haehnchen", "HA" },
  { "Kotelett", "KO" },  { "Schnitzel", "SC" },
  { "Fisch", "FI" },  { "Garnelen", "GA" },
  { "Eieruhr", "EI" },  { "Wecker", "WE" },
  { "Milchtopf", "MI" },  { "Wasserkessel", "WA" },
  { "Espresso", "ES" },  { "Einmachglas", "EI" },
  { "Dampfgarer", "DA" },
};

// Bilddaten: src/gen/icons_data.c, erzeugt aus assets/icons/ per tools/convert_icons.py
#define ICONS_GROSS ICON_IMG_64
#define ICONS_KLEIN ICON_IMG_36

const char *icon_name(uint8_t id)   { return ICONS[id % ICON_COUNT].name; }
const char *icon_abbrev(uint8_t id) { return ICONS[id % ICON_COUNT].abbrev; }

lv_obj_t *icon_create(lv_obj_t *parent, int size) {
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_size(box, size, size);
  lv_obj_set_style_bg_opa(box, LV_OPA_0, 0);      // kein Kreis, kein Rahmen -
  lv_obj_set_style_border_width(box, 0, 0);       // nur das eingefaerbte Icon
  lv_obj_set_style_pad_all(box, 0, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *img = lv_img_create(box);
  lv_obj_center(img);
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *lbl = lv_label_create(box);
  lv_obj_center(lbl);
  lv_obj_set_style_text_font(lbl, size >= 70 ? &lv_font_montserrat_28
                                  : size >= 44 ? &lv_font_montserrat_20
                                               : &lv_font_montserrat_14, 0);  // nur Platzhalter
  lv_label_set_text(lbl, "");
  return box;
}

void icon_set(lv_obj_t *box, uint8_t id, lv_color_t color) {
  if (!box) return;
  lv_obj_t *img = lv_obj_get_child(box, 0);
  lv_obj_t *lbl = lv_obj_get_child(box, 1);
  int size = lv_obj_get_width(box);
  const lv_img_dsc_t *src = (size >= 64 ? ICONS_GROSS : ICONS_KLEIN)[id % ICON_COUNT];

  if (src) {
    lv_img_set_src(img, src);
    lv_obj_set_style_img_recolor(img, color, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(lbl, icon_abbrev(id));
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_clear_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  }
}
