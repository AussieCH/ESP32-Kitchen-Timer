// Icon-Registry mit 32 Plaetzen.
//
// Die Grafiken liegen als reine Alphakanaele vor (weisse Strichzeichnung ->
// Deckkraft); eingefaerbt wird zur Laufzeit in der Farbe des jeweiligen Timers.
// Neu erzeugen: python3 tools/convert_icons.py assets/icons
// Reihenfolge = Icon-ID und steckt in gespeicherten Vorlagen -> nicht umsortieren.
#include "icons.h"
#include "gen/font_ui.h"
#include "lang.h"
#include "gen/icons_data.h"

struct IconDef { const char *de; const char *en; const char *abbrev; };

static const IconDef ICONS[ICON_COUNT] = {
  { "Pasta", "Pasta", "PA" },
  { "Ei", "Egg", "EI" },
  { "Hefezopf", "Sweet loaf", "HE" },
  { "Reis", "Rice", "RE" },
  { "Kartoffeln", "Potatoes", "KA" },
  { "Gemüse", "Vegetables", "GE" },
  { "Brokkoli", "Broccoli", "BR" },
  { "Spargel", "Asparagus", "SP" },
  { "Suppe", "Soup", "SU" },
  { "Kochtopf", "Pot", "KO" },
  { "Kochen", "Boiling", "KO" },
  { "Braten", "Roast", "BR" },
  { "Bratpfanne", "Frying pan", "BR" },
  { "Teig", "Dough", "TE" },
  { "Gehen lassen", "Proving", "GL" },
  { "Brot", "Bread", "BR" },
  { "Kuchen", "Cake", "KU" },
  { "Kekse", "Cookies", "KE" },
  { "Eintopf", "Stew", "EI" },
  { "Steak", "Steak", "ST" },
  { "Filet", "Fillet", "FI" },
  { "Hähnchen", "Chicken", "HA" },
  { "Kotelett", "Chop", "KO" },
  { "Schnitzel", "Cutlet", "SC" },
  { "Fisch", "Fish", "FI" },
  { "Garnelen", "Prawns", "GA" },
  { "Eieruhr", "Egg timer", "EI" },
  { "Wecker", "Timer", "WE" },
  { "Milchtopf", "Milk pan", "MI" },
  { "Wasserkessel", "Kettle", "WA" },
  { "Espresso", "Espresso", "ES" },
  { "Einmachglas", "Jar", "EI" },
  { "Dampfgarer", "Steamer", "DA" },
};

// Bilddaten: src/gen/icons_data.c, erzeugt aus assets/icons/ per tools/convert_icons.py
#define ICONS_GROSS ICON_IMG_64
#define ICONS_KLEIN ICON_IMG_36

const char *icon_name(uint8_t id) {
  const IconDef &d = ICONS[id % ICON_COUNT];
  return lang_get() == LANG_EN ? d.en : d.de;
}
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
  lv_obj_set_style_text_font(lbl, size >= 70 ? &font_ui_28
                                  : size >= 44 ? &font_ui_20
                                               : &font_ui_14, 0);  // nur Platzhalter
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
