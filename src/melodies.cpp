// Notentabellen. Frequenzen in Hz, Dauer in ms. Quelle: Toene.txt.
#include "melodies.h"
#include "lang.h"

#define G3  196
#define C4  262
#define CS4 277
#define D4  294
#define DS4 311
#define E4  330
#define F4  349
#define FS4 370
#define G4  392
#define GS4 415
#define A4  440
#define AS4 466
#define B4  494
#define C5  523
#define CS5 554
#define D5  587
#define DS5 622
#define E5  659
#define F5  698
#define FS5 740
#define G5  784
#define GS5 831
#define A5  880
#define AS5 932
#define B5  988
#define C6  1047
#define CS6 1109
#define D6  1175
#define DS6 1245
#define E6  1319
#define F6  1397
#define G6  1568

static const Note m01[] = { {G4,250},{G4,250},{A4,450},{G4,450},{C5,450},{B4,700},{G4,250},{G4,250},{A4,450} };
static const Note m02[] = { {C4,330},{D4,330},{E4,330},{C4,330},{C4,330},{D4,330},{E4,330},{C4,330},{E4,330},{F4,330},{G4,660} };
static const Note m03[] = { {C4,300},{D4,300},{E4,300},{F4,300},{G4,600},{G4,300},{A4,300},{A4,300},{A4,300},{A4,300},{G4,500} };
static const Note m04[] = { {E4,300},{E4,300},{E4,600},{E4,300},{E4,300},{E4,600},{E4,300},{G4,300},{C4,300},{D4,300},{E4,700} };
static const Note m05[] = { {E4,250},{E4,250},{F4,250},{G4,250},{G4,250},{F4,250},{E4,250},{D4,250},{C4,250},{C4,250},{D4,250},{E4,250},{E4,375},{D4,125},{D4,500} };
static const Note m06[] = { {G4,300},{E4,300},{E4,600},{F4,300},{D4,300},{D4,600},{C4,300},{D4,300},{E4,300},{F4,300},{G4,600} };
static const Note m07[] = { {G4,260},{A4,260},{G4,260},{F4,260},{E4,260},{F4,260},{G4,520},{D4,260},{E4,260},{F4,520},{E4,260},{F4,260},{G4,520} };
static const Note m08[] = { {C4,300},{E4,300},{F4,300},{G4,600},{C4,300},{E4,300},{F4,300},{G4,600},{C4,300},{E4,300},{F4,300},{G4,600} };
static const Note m09[] = { {C4,300},{C4,300},{C4,300},{D4,300},{E4,600},{D4,600},{C4,300},{E4,300},{D4,300},{D4,300},{C4,600} };
static const Note m10[] = { {E5,250},{DS5,250},{E5,250},{DS5,250},{E5,250},{B4,300},{D5,300},{C5,300},{A4,800} };
static const Note m11[] = { {G4,300},{D4,300},{G4,300},{B4,300},{D5,600},{G5,600},{D5,300},{G5,900} };
static const Note m12[] = { {E4,400},{G4,400},{B4,400},{E5,600},{D5,400},{B4,400},{G4,400},{D4,400},{E4,400} };
static const Note m13[] = { {B4,250},{A4,250},{GS4,250},{A4,250},{C5,500},{B4,250},{A4,250},{B4,250},{E5,500},{D5,250},{C5,250},{B4,500} };
static const Note m14[] = { {D4,300},{D4,300},{D4,300},{D4,300},{CS4,300},{D4,300},{E4,300},{F4,500},{E4,300},{D4,300},{CS4,600} };
static const Note m15[] = { {E4,400},{C4,400},{D4,400},{G3,700},{G3,400},{D4,400},{E4,400},{C4,500} };
static const Note m16[] = { {E5,250},{D5,250},{FS4,300},{GS4,300},{CS5,250},{B4,250},{D4,300},{E4,300},{B4,250},{A4,250},{CS4,300},{E4,300},{A4,500} };
static const Note m17[] = { {E5,200},{B4,200},{C5,200},{D5,200},{C5,200},{B4,200},{A4,200},{A4,200},{C5,200},{E5,200},{D5,200},{C5,200},
                            {B4,200},{C5,200},{D5,200},{E5,200},{C5,200},{A4,200},{A4,400} };
static const Note m18[] = { {C5,250},{E5,250},{G5,250},{C6,400},{REST,200},{G5,250},{C6,250},{E6,400},{G6,650},{C6,400} };
static const Note m19[] = { {G5,500},{E5,700},{C5,500},{REST,300},{G5,500},{E5,700},{C5,500} };
static const Note m20[] = { {C5,300},{E5,300},{G5,300},{C6,500},{REST,200},{C5,300},{E5,300},{G5,300},{C6,700} };

#define M(de, en, tab) { de, en, tab, (uint16_t)(sizeof(tab) / sizeof(Note)) }

const Melody MELODIES[] = {
  M("Geburtstag", "Birthday",    m01),  M("Bruder Jakob", "Brother John",  m02),  M("Entchen", "Little Ducks",       m03),
  M("Jingle Bells", "Jingle Bells",  m04),  M("Ode Freude", "Ode to Joy",    m05),  M("Hänschen", "Little Hans",     m06),
  M("London Bridge", "London Bridge", m07),  M("Saints", "Saints",        m08),  M("Au clair", "Au clair",      m09),
  M("Für Elise", "Für Elise",    m10),  M("Nachtmusik", "Night Music",    m11),  M("Morgen", "Morning Mood",        m12),
  M("Türk. Marsch", "Turkish March", m13),  M("Habanera", "Habanera",      m14),  M("Westminster", "Westminster",   m15),
  M("Gran Vals", "Gran Vals",     m16),  M("Tetris", "Tetris",        m17),  M("Game Start", "Game Start",    m18),
  M("Türgong", "Doorbell",      m19),  M("Alarm", "Alarm",         m20),
};
const int MELODY_COUNT = sizeof(MELODIES) / sizeof(MELODIES[0]);

static const Note m_test[] = { {C5,120},{E5,120},{G5,220} };
const Melody MELODY_TEST = M("Test", "Test", m_test);

const char *melody_name(const Melody *m) {
  return lang_get() == LANG_EN ? m->en : m->de;
}
