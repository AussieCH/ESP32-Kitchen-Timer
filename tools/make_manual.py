#!/usr/bin/env python3
"""Erzeugt die Bedienungsanleitung als PDF - deutsch und englisch.

Die Screenshots stammen aus dem Host-Simulator (tools/sim), sind also
tatsaechlich das, was auf dem Geraet zu sehen ist - keine Zeichnungen. Der
englische Satz braucht den englischen Simulatorlauf:

    ./tools/sim/build.sh && ./tools/sim/build.sh en
    python3 tools/make_manual.py       -> RONDO-Anleitung.pdf
    python3 tools/make_manual.py en    -> RONDO-Manual.pdf

Beide Sprachen stehen im Quelltext Zeile fuer Zeile nebeneinander - T(de, en) -
wie der Textkatalog der Firmware in src/lang.cpp. Uebersetzt man getrennte
Dateien, laufen sie beim naechsten Feature auseinander.
"""
import os, math, sys
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Paragraph,
                                Spacer, Image, Table, TableStyle, KeepTogether)
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.fonts import addMapping

EN = len(sys.argv) > 1 and sys.argv[1] == "en"

def T(de, en):
    """Ein Textpaar. Der Aufruf steht dort, wo der Text hingehoert."""
    return en if EN else de

OUT    = T("RONDO-Anleitung.pdf", "RONDO-Manual.pdf")
SHOTS  = "tools/sim/out"
PREFIX = T("", "en-")          # der Simulator legt den englischen Satz mit Praefix ab
VARFONT = os.path.expanduser("~/Library/Fonts/Rubik-VariableFont_wght.ttf")
FONTDIR = "tools/.fonts"


def setup_fonts():
    """Rubik einbetten - dieselbe Schrift wie die Ziffern auf dem Geraet.

    Ohne eingebettete Schrift ist das Ergebnis Glueckssache: die eingebauten
    PDF-Schriften werden vom Betrachter ersetzt, und hier landeten Helvetica und
    Helvetica-Bold auf demselben Schnitt - im ganzen Dokument war nichts fett.
    Rubik liegt nur als variable Schrift vor, deshalb schneiden wir uns mit
    fontTools zwei feste Gewichte heraus.
    """
    reg, bold = FONTDIR + "/Rubik-Regular.ttf", FONTDIR + "/Rubik-Bold.ttf"
    if not (os.path.exists(reg) and os.path.exists(bold)):
        if not os.path.exists(VARFONT):
            print("Rubik nicht gefunden - weiche auf Helvetica aus (ohne Fettschnitt!)")
            return "Helvetica", "Helvetica-Bold"
        os.makedirs(FONTDIR, exist_ok=True)
        from fontTools.ttLib import TTFont as FTFont
        from fontTools.varLib.instancer import instantiateVariableFont
        for path, weight in ((reg, 400), (bold, 700)):
            f = FTFont(VARFONT)
            # updateFontNames=True ist Pflicht: sonst heisst auch der fette
            # Schnitt intern "Rubik Regular", reportlab bettet ihn unter
            # demselben Namen ein und im PDF ist alles gleich dick.
            instantiateVariableFont(f, {"wght": weight}, inplace=True, updateFontNames=True)
            f.save(path)
    pdfmetrics.registerFont(TTFont("Rubik", reg))
    pdfmetrics.registerFont(TTFont("Rubik-Bold", bold))
    addMapping("Rubik", 0, 0, "Rubik")        # normal
    addMapping("Rubik", 1, 0, "Rubik-Bold")   # fett
    addMapping("Rubik", 0, 1, "Rubik")        # kursiv gibt es nicht
    addMapping("Rubik", 1, 1, "Rubik-Bold")
    return "Rubik", "Rubik-Bold"


FONT, FONT_B = setup_fonts()

ORANGE = colors.HexColor("#EE7C25")
CREAM  = colors.HexColor("#FDF4E9")
INK    = colors.HexColor("#1A1A1A")
GREY   = colors.HexColor("#5A5A5A")
RULE   = colors.HexColor("#DDDDDD")

W, H = A4
MARGIN = 20 * mm

# ---------------------------------------------------------------- Stile
def style(name, size, leading, color=INK, font=None, space_before=0, space_after=0, left=0):
    return ParagraphStyle(name, fontName=font or FONT, fontSize=size, leading=leading,
                          textColor=color, spaceBefore=space_before,
                          spaceAfter=space_after, alignment=TA_LEFT, leftIndent=left)

S_H1   = style("h1", 19, 23, ORANGE, FONT_B, 10, 5)
S_H2   = style("h2", 12.5, 16, INK, FONT_B, 9, 3)
S_BODY = style("body", 10.5, 16, INK, space_after=5)
S_NOTE = style("note", 9.5, 14, GREY, space_after=4)
S_STEP = style("step", 10.5, 16, INK, space_after=3, left=7*mm)
S_CAP  = style("cap", 8.5, 11, GREY)

# ---------------------------------------------------------------- Titelseite
def draw_logo(c, cx, cy, ring_r, dot_r, highlight=0):
    for i in range(13):
        a = -math.pi / 2 + i * 2 * math.pi / 13
        c.setFillColor(ORANGE if i == highlight else CREAM)
        c.circle(cx + ring_r * math.cos(a), cy - ring_r * math.sin(a), dot_r, stroke=0, fill=1)

def title_page(c, doc):
    c.saveState()
    c.setFillColor(colors.black)
    c.rect(0, 0, W, H, stroke=0, fill=1)

    cx, cy = W / 2, H * 0.60
    # Verhaeltnis aus der Vorlage: Punktdurchmesser = 0.28 x Ringradius
    draw_logo(c, cx, cy, 42 * mm, 5.9 * mm)

    # Sperrsatz wie im Logo: die Breite muss von Hand gerechnet werden, damit
    # der Schriftzug trotz Buchstabenabstand mittig sitzt.
    from reportlab.pdfbase.pdfmetrics import stringWidth
    word, size, gap = "RONDO", 33, 12
    tw = stringWidth(word, FONT, size) + gap * (len(word) - 1)
    to = c.beginText(cx - tw / 2, cy - 10)
    to.setFont(FONT, size)
    to.setFillColor(CREAM)
    to.setCharSpace(gap)
    to.textOut(word)
    c.drawText(to)

    # Wichtig: der Buchstabenabstand von oben bleibt sonst im Zustand haengen
    # und sperrt auch die Zeilen darunter.
    def centred(text, size, y, col):
        w = stringWidth(text, FONT, size)
        t = c.beginText(cx - w / 2, y)
        t.setFont(FONT, size)
        t.setFillColor(col)
        t.setCharSpace(0)
        t.textOut(text)
        c.drawText(t)

    centred(T("Küchentimer", "Kitchen timer"), 14, H * 0.24, CREAM)
    centred(T("Bedienungsanleitung", "User manual"), 10.5, H * 0.24 - 20,
            colors.HexColor("#9A9A9A"))
    c.restoreState()

def content_page(c, doc):
    c.saveState()
    c.setFont(FONT, 8)
    c.setFillColor(colors.HexColor("#9A9A9A"))
    c.drawString(MARGIN, H - 12 * mm, T("RONDO  ·  Küchentimer", "RONDO  ·  Kitchen timer"))
    c.setStrokeColor(RULE)
    c.setLineWidth(0.5)
    c.line(MARGIN, H - 14 * mm, W - MARGIN, H - 14 * mm)
    c.drawRightString(W - MARGIN, 12 * mm, str(doc.page - 1))
    # kleines Logo als Fusszeichen
    draw_logo(c, MARGIN + 3, 12 * mm + 3, 0, 1.6, highlight=-1)
    c.restoreState()

# ---------------------------------------------------------------- Bausteine
def shot(name, width=47 * mm):
    return Image(os.path.join(SHOTS, PREFIX + name), width=width, height=width)

def screen_block(img, title, paras, caption=None, extra=None):
    """Bild links, Text rechts - haelt beides auf derselben Seite zusammen."""
    right = [Paragraph(title, S_H2)] + [Paragraph(p, S_BODY) for p in paras] + (extra or [])
    left = [img] + ([Paragraph(caption, S_CAP)] if caption else [])
    t = Table([[left, right]], colWidths=[51 * mm, None])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (0, 0), 0),
        ("LEFTPADDING", (1, 0), (1, 0), 7 * mm),
        ("RIGHTPADDING", (-1, -1), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 8),
    ]))
    return KeepTogether(t)

def bullets(items, s=S_BODY):
    return [Paragraph("•&nbsp;&nbsp;" + i, ParagraphStyle("b", parent=s, leftIndent=6 * mm,
                                                          firstLineIndent=-4 * mm)) for i in items]

def build():
    doc = BaseDocTemplate(OUT, pagesize=A4,
                          leftMargin=MARGIN, rightMargin=MARGIN,
                          topMargin=22 * mm, bottomMargin=18 * mm,
                          title=T("RONDO Küchentimer – Bedienungsanleitung",
                                  "RONDO kitchen timer – user manual"),
                          author="RONDO")
    frame = Frame(MARGIN, 18 * mm, W - 2 * MARGIN, H - 40 * mm, id="f")
    doc.addPageTemplates([
        PageTemplate(id="title", frames=[Frame(0, 0, W, H, id="t")], onPage=title_page),
        PageTemplate(id="content", frames=[frame], onPage=content_page),
    ])
    st = []
    from reportlab.platypus import NextPageTemplate, PageBreak
    st += [NextPageTemplate("content"), PageBreak()]
    return doc, st

# ---------------------------------------------------------------- Inhalt
def content(st):
    st.append(Paragraph(T("Willkommen", "Welcome"), S_H1))
    st.append(Paragraph(T(
        "RONDO ist eine Küchenuhr. Sie stellen eine Zeit ein, und wenn sie abgelaufen ist, "
        "meldet sich das Gerät — hörbar, sichtbar und spürbar. Mehrere Zeiten gleichzeitig sind "
        "kein Problem: Kartoffeln, Ei und Braten laufen nebeneinander, jedes in einer eigenen Farbe.",
        "RONDO is a kitchen timer. You set a time, and when it runs out the device lets you know — "
        "you hear it, you see it and you feel it. Several times at once are no trouble: potatoes, "
        "egg and roast run side by side, each in its own colour."), S_BODY))
    st.append(Paragraph(T(
        "Es braucht kein Internet, kein Konto und keine App. Alles passiert auf dem Gerät selbst. "
        "Sie können es also einfach einstecken und benutzen.",
        "It needs no internet, no account and no app. Everything happens on the device itself. "
        "So you can simply plug it in and use it."), S_BODY))

    st.append(Paragraph(T("Das Gerät", "The device"), S_H2))
    st += bullets([
        T("<b>Der runde Bildschirm</b> zeigt alles an — und reagiert auf Berührung, wie ein Handy.",
          "<b>The round screen</b> shows everything — and responds to touch, like a phone."),
        T("<b>Der Ring aussen herum lässt sich drehen.</b> Er rastet spürbar ein: ein Klick, ein Schritt. "
          "Drücken lässt er sich nicht — alles, was gedrückt werden will, tippen Sie auf dem Bildschirm an.",
          "<b>The ring around the edge turns.</b> It clicks into place as you feel it: one click, one step. "
          "It cannot be pressed — anything that wants pressing, you tap on the screen."),
        T("<b>Hinter dem Rand leuchten kleine Lämpchen</b> im Kreis. Sie zeigen aus der Ferne, "
          "wie viel Zeit noch übrig ist.",
          "<b>Small lights glow behind the rim</b>, arranged in a circle. They show from across the "
          "room how much time is left."),
        T("<b>Ein Lautsprecher</b> für die Melodie und <b>ein kleiner Motor</b>, der das Gerät leise "
          "brummen lässt — so merken Sie den Alarm auch, wenn es laut ist in der Küche.",
          "<b>A speaker</b> for the melody and <b>a small motor</b> that makes the device buzz quietly — "
          "so you notice the alarm even when the kitchen is loud."),
    ])

    st.append(Paragraph(T("Die drei Handgriffe", "The three gestures"), S_H1))
    st.append(Paragraph(T(
        "Mehr müssen Sie sich nicht merken. Diese drei gelten überall gleich:",
        "There is nothing else to memorise. These three work the same everywhere:"), S_BODY))
    t = Table([
        [Paragraph(T("<b>Wischen</b><br/>nach links oder rechts", "<b>Swipe</b><br/>left or right"), S_BODY),
         Paragraph(T("wechselt die <b>Seite</b>. Die Punkte unten am Rand zeigen, auf welcher Sie gerade sind.",
                     "changes the <b>page</b>. The dots along the bottom show which one you are on."), S_BODY)],
        [Paragraph(T("<b>Ring drehen</b>", "<b>Turn the ring</b>"), S_BODY),
         Paragraph(T("ändert <b>etwas auf der Seite</b>, auf der Sie sind — eine Zahl, eine Auswahl, "
                     "oder blättert durch eine Liste. Die Seite wechselt der Ring nie.",
                     "changes <b>something on the page</b> you are on — a number, a choice, or it "
                     "scrolls a list. The ring never changes the page."), S_BODY)],
        [Paragraph(T("<b>Tippen</b>", "<b>Tap</b>"), S_BODY),
         Paragraph(T("wählt aus und bestätigt. Was Sie angetippt haben, wird gelb umrandet — "
                     "das ist dann das, was der Ring verstellt.",
                     "selects and confirms. Whatever you tapped gets a yellow outline — and that is "
                     "what the ring will change."), S_BODY)],
    ], colWidths=[45 * mm, None])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"),
                           ("LEFTPADDING", (0, 0), (0, -1), 0),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
                           ("LINEBELOW", (0, 0), (-1, -2), 0.4, RULE)]))
    st.append(t)
    st.append(Paragraph(T(
        "Wenn Sie sich verlaufen: einfach so lange wischen, bis Sie wieder auf einer bekannten Seite sind. "
        "Kaputtmachen können Sie nichts.",
        "If you get lost: just keep swiping until you are back on a page you recognise. "
        "You cannot break anything."), S_NOTE))

    st.append(Paragraph(T("Die acht Seiten", "The eight pages"), S_H1))
    st.append(Paragraph(T(
        "Die Seiten liegen nebeneinander wie Bilder in einem Album. Nach dem Einschalten stehen Sie "
        "auf <b>Läuft gerade</b>. Von dort nach rechts wischen führt zur Übersicht, nach links zu allem anderen.",
        "The pages sit side by side like pictures in an album. After switching on you are on "
        "<b>Running now</b>. Swiping right from there leads to the overview, left to everything else."), S_BODY))
    seiten = [
        (T("Übersicht", "Overview"), T("alle laufenden Zeiten auf einen Blick",
                                       "every running time at a glance")),
        (T("Läuft gerade", "Running now"), T("die Startseite: eine Zeit gross, mit den Knöpfen dazu",
                                             "the home page: one time large, with its buttons")),
        (T("Neuer Timer", "New timer"), T("hier stellen Sie eine neue Zeit ein",
                                          "this is where you set a new time")),
        (T("Eieruhr", "Egg timer"), T("rechnet die Kochzeit für Eier aus",
                                      "works out the cooking time for eggs")),
        (T("Grill-Thermometer", "Grill thermometer"), T("zeigt die Temperatur im Fleisch",
                                                        "shows the temperature inside the meat")),
        (T("Stoppuhr", "Stopwatch"), T("zählt vorwärts statt rückwärts",
                                       "counts up instead of down")),
        (T("Vorlagen", "Presets"), T("gespeicherte Zeiten, mit einem Tipp gestartet",
                                     "saved times, started with a single tap")),
        (T("Einstellungen", "Settings"), T("Helligkeit, Lautstärke, Sprache, Akku",
                                           "brightness, volume, language, battery")),
    ]
    rows = [[Paragraph(f"<b>{n}</b>", S_BODY), Paragraph(b, S_BODY)] for n, b in seiten]
    t = Table(rows, colWidths=[38 * mm, None])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"),
                           ("LEFTPADDING", (0, 0), (0, -1), 0),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
    st.append(t)

    st.append(Paragraph(T("Ihr erster Timer", "Your first timer"), S_H1))
    st.append(screen_block(
        shot("3-neuer-timer.png"), T("So stellen Sie fünf Minuten ein", "Setting five minutes"),
        [T("Wischen Sie von der Startseite <b>zweimal nach links</b>, bis oben „Neuer Timer“ steht.",
           "From the home page, swipe <b>left twice</b> until the top reads “New timer”.")],
        T("Die Seite „Neuer Timer“", "The “New timer” page"),
        extra=[
            Paragraph(T("1.&nbsp;&nbsp;Tippen Sie auf die <b>mittleren zwei Ziffern</b> — das sind die "
                        "Minuten. Sie werden gelb.",
                        "1.&nbsp;&nbsp;Tap the <b>middle two digits</b> — those are the minutes. "
                        "They turn yellow."), S_STEP),
            Paragraph(T("2.&nbsp;&nbsp;Drehen Sie den Ring, bis <b>05</b> dasteht. Ein Klick ist eine Minute.",
                        "2.&nbsp;&nbsp;Turn the ring until it reads <b>05</b>. One click is one minute."), S_STEP),
            Paragraph(T("3.&nbsp;&nbsp;Tippen Sie auf <b>Start</b>. Fertig.",
                        "3.&nbsp;&nbsp;Tap <b>Start</b>. Done."), S_STEP),
        ]))
    st.append(Paragraph(T(
        "Noch schneller geht es mit den vier Knöpfen darunter: <b>3, 5, 10 und 15 Minuten</b> mit einem Tipp.",
        "The four buttons below are quicker still: <b>3, 5, 10 and 15 minutes</b> with one tap."), S_BODY))
    st.append(Paragraph(T(
        "Die beiden Knöpfe in der Mitte sind Geschmackssache: links wählen Sie die <b>Melodie</b>, die am Ende "
        "spielt (sie wird beim Auswählen gleich vorgespielt), rechts ein <b>Bild</b> — Ei, Pasta, Brot und so "
        "weiter. Das Bild hilft später, die Timer auseinanderzuhalten.",
        "The two buttons in the middle are a matter of taste: on the left you pick the <b>melody</b> that "
        "plays at the end (it is played back as you choose), on the right a <b>picture</b> — egg, pasta, "
        "bread and so on. The picture is what tells your timers apart later."), S_BODY))

    st.append(Paragraph(T("Wenn die Zeit um ist", "When the time is up"), S_H1))
    st.append(screen_block(
        shot("6-alarm.png"), T("Der Alarm", "The alarm"),
        [T("Das Gerät holt die abgelaufene Zeit von selbst nach vorne, egal wo Sie gerade waren. "
           "Der Bildschirm blinkt in der Farbe des Timers, das Gerät brummt, die Melodie spielt "
           "und die Lämpchen blinken mit.",
           "The device brings the finished time to the front by itself, wherever you happened to be. "
           "The screen flashes in that timer's colour, the device buzzes, the melody plays and the "
           "lights blink along."),
         T("<b>Zum Ausschalten tippen Sie einfach mitten auf den Bildschirm.</b> Die ganze Fläche ist "
           "die Taste — das klappt auch mit dem Handrücken, wenn die Finger teigig sind.",
           "<b>To switch it off, simply tap the middle of the screen.</b> The whole area is the button — "
           "which also works with the back of your hand when your fingers are covered in dough."),
         T("Brauchen Sie doch noch etwas Zeit, tippen Sie auf <b>+5 Min</b>.",
           "If you need a little longer after all, tap <b>+5 min</b>.")],
        T("Ein abgelaufener Timer", "A finished timer")))
    st.append(Paragraph(T(
        "Wenn niemand reagiert, hört der Alarm nach fünf Minuten von selbst auf. Der Timer bleibt "
        "dann als „abgelaufen“ stehen, damit Sie sehen, dass etwas fertig geworden ist.",
        "If nobody reacts, the alarm stops by itself after five minutes. The timer then stays on screen "
        "as “time is up”, so you can see that something finished."), S_NOTE))

    st.append(Paragraph(T("Mehrere Zeiten gleichzeitig", "Several times at once"), S_H1))
    st.append(screen_block(
        shot("2-aktiv-drei.png"), T("Läuft gerade", "Running now"),
        [T("Hier steht immer <b>eine</b> Zeit gross im Bild. Der farbige Bogen aussen herum leert sich, "
           "während die Zeit abläuft — so sehen Sie den Fortschritt, ohne die Zahl zu lesen.",
           "One time is always shown large here. The coloured arc around the edge empties as the time "
           "runs down — so you see the progress without reading the number."),
         T("Laufen mehrere Zeiten, steht unter den Knöpfen zum Beispiel <b>1 / 3</b>. Mit dem <b>Ring</b> "
           "oder mit Wischen nach oben und unten blättern Sie durch.",
           "When several times are running, the buttons show something like <b>1 / 3</b> beneath them. "
           "Use the <b>ring</b>, or swipe up and down, to page through them."),
         T("<b>Jede Zeit hat ihre eigene Farbe.</b> Der Leuchtring am Gerät nimmt immer die Farbe der Zeit an, "
           "die Sie gerade ansehen.",
           "<b>Every time has its own colour.</b> The ring of lights always takes the colour of the time "
           "you are currently looking at."),
         T("Läuft gerade nichts, zeigt RONDO sein Zeichen: der orange Punkt wandert langsam im Kreis. "
           "Ein Tipp darauf führt direkt zum neuen Timer.",
           "When nothing is running, RONDO shows its mark: the orange dot travels slowly around the "
           "circle. A tap on it takes you straight to a new timer.")],
        T("Drei Timer, hier der zweite", "Three timers, the second one shown")))
    st.append(screen_block(
        shot("2b-uebersicht.png"), T("Übersicht", "Overview"),
        [T("Eine Seite nach rechts liegt die Liste aller laufenden Zeiten. Ein Tipp auf eine Zeile "
           "holt sie gross nach vorne.",
           "One page to the right is the list of every running time. A tap on a row brings it to the "
           "front, large."),
         T("Praktisch, wenn drei Sachen gleichzeitig kochen und Sie nur kurz schauen wollen, "
           "was als Nächstes fertig wird.",
           "Handy when three things are cooking at once and you only want a quick look at what will "
           "be ready next.")],
        T("Alles auf einen Blick", "Everything at a glance")))

    st.append(Paragraph(T("Die anderen Seiten", "The other pages"), S_H1))
    st.append(screen_block(
        shot("3c-eieruhr.png"), T("Eieruhr", "Egg timer"),
        [T("Sagen Sie dem Gerät, <b>wie gross</b> das Ei ist, ob es <b>aus dem Kühlschrank</b> kommt "
           "und wie Sie es <b>mögen</b> — weich, wachsweich oder hart. Die Kochzeit rechnet es aus.",
           "Tell the device <b>how large</b> the egg is, whether it comes <b>out of the fridge</b>, and "
           "how you <b>like it</b> — soft, medium or hard. It works out the cooking time."),
         T("Zeile antippen, Ring drehen, <b>Kochen</b> tippen. Die Zeit läuft dann wie jede andere.",
           "Tap a row, turn the ring, tap <b>Cook</b>. The time then runs like any other.")],
        T("Ein mittleres Ei aus dem Kühlschrank, weich", "A medium egg from the fridge, soft")))
    st.append(screen_block(
        shot("3e-fuehler.png"), T("Grill-Thermometer", "Grill thermometer"),
        [T("Haben Sie ein <b>MEATER</b>-Thermometer, zeigt RONDO an, wie warm es im Fleisch ist — "
           "gross die <b>Kerntemperatur</b>, darunter die Temperatur im Garraum und der Ladestand "
           "des Fühlers.",
           "If you have a <b>MEATER</b> thermometer, RONDO shows how warm it is inside the meat — the "
           "<b>core temperature</b> large, and below it the temperature around the meat and the "
           "probe's battery level."),
         T("Sie müssen nichts einrichten: <b>Fühler aus der Ladeschale nehmen</b>, und RONDO "
           "verbindet sich von selbst. Der Punkt unten wird grün, sobald Werte ankommen.",
           "There is nothing to set up: <b>take the probe out of its charging case</b> and RONDO "
           "connects by itself. The dot turns green as soon as readings arrive."),
         T("Der Fühler spricht immer nur mit einem Gerät — und das ist hier RONDO. Die "
           "Hersteller-App bleibt aussen vor, solange das Thermometer verbunden ist. So ist es "
           "gedacht: eine Zahl, ein Alarm, kein Assistent.",
           "The probe only ever talks to one device — and here that is RONDO. The manufacturer's app "
           "stays out while the thermometer is connected. That is the intent: a number, an alarm, "
           "no assistant.")],
        T("Ein Braten bei 62,4 °C", "A roast at 62.4 °C"),
        extra=[
            Paragraph(T("<b>Damit das Fleisch Sie ruft</b>", "<b>Letting the meat call you</b>"), S_H2),
            Paragraph(T("Drehen Sie am Ring, bis die gewünschte <b>Zieltemperatur</b> im Kasten steht — "
                        "rechts daneben steht, was das bedeutet: <i>englisch</i>, <i>rosa</i>, "
                        "<i>medium</i>, <i>durch</i>. Dann auf <b>Alarm ein</b> tippen.",
                        "Turn the ring until the <b>target temperature</b> you want appears in the box — "
                        "next to it stands what that means: <i>rare</i>, <i>medium rare</i>, "
                        "<i>medium</i>, <i>well done</i>. Then tap <b>Alarm on</b>."), S_BODY),
            Paragraph(T("Ist die Temperatur erreicht, meldet sich RONDO genau wie bei einer "
                        "abgelaufenen Zeit: der Bildschirm blinkt, das Gerät brummt, die Melodie "
                        "spielt. <b>Stopp</b> beendet es.",
                        "Once the temperature is reached, RONDO announces it exactly as it does a "
                        "finished time: the screen flashes, the device buzzes, the melody plays. "
                        "<b>Stop</b> ends it."), S_BODY),
        ]))
    st.append(screen_block(
        shot("3d-stoppuhr.png"), T("Stoppuhr", "Stopwatch"),
        [T("Zählt vorwärts statt rückwärts — für „wie lange köchelt das jetzt schon?“.",
           "Counts up instead of down — for “how long has this been simmering?”."),
         T("Sie läuft im Hintergrund weiter, auch wenn Sie wegwischen. Solange sie läuft, taucht sie "
           "auch auf der Seite „Läuft gerade“ und in der Übersicht auf.",
           "It keeps running in the background even when you swipe away. While it runs, it also "
           "appears on the “Running now” page and in the overview.")],
        T("Start, Pause, Zurück", "Start, pause, reset")))
    st.append(screen_block(
        shot("4-vorlagen.png"), T("Vorlagen", "Presets"),
        [T("Jede Zeit, die Sie starten, merkt sich das Gerät automatisch. Hier stehen sie alle.",
           "The device remembers every time you start, automatically. They are all listed here."),
         T("<b>Antippen startet</b> die Zeit sofort. <b>Länger draufhalten</b> öffnet ein kleines Menü "
           "zum Ändern oder Löschen.",
           "<b>Tapping starts</b> the time straight away. <b>Holding it longer</b> opens a small menu "
           "for changing or deleting it.")],
        T("Gespeicherte Zeiten", "Saved times")))
    st.append(screen_block(
        shot("5-einstellungen.png"), T("Einstellungen", "Settings"),
        [T("<b>Helligkeit</b> und <b>Lautstärke</b>: Zeile antippen, dann mit dem Ring einstellen.",
           "<b>Brightness</b> and <b>volume</b>: tap the row, then set it with the ring."),
         T("<b>Sprache</b>: dieselbe Bedienung — Zeile antippen, Ring drehen. Zur Wahl stehen "
           "<b>Deutsch</b> und <b>English</b>; die ganze Anzeige wechselt sofort mit.",
           "<b>Language</b>: the same handling — tap the row, turn the ring. The choice is "
           "<b>Deutsch</b> or <b>English</b>; the whole display changes over immediately."),
         T("<b>Alarm testen</b> spielt einmal vor, was passiert, wenn eine Zeit abläuft.",
           "<b>Test alarm</b> demonstrates once what happens when a time runs out."),
         T("Ganz unten steht der <b>Akkustand</b>. Hängt das Gerät am Strom, zeigt es die Spannung "
           "statt einer Prozentzahl — beim Laden wäre eine Prozentangabe geraten.",
           "At the very bottom is the <b>battery level</b>. While the device is plugged in it shows "
           "the voltage instead of a percentage — during charging a percentage would be guesswork.")],
        T("Helligkeit, Lautstärke und Sprache", "Brightness, volume and language")))

    st.append(Paragraph(T("Der Leuchtring", "The ring of lights"), S_H1))
    st.append(Paragraph(T(
        "Die Lämpchen am Rand sind nicht Dekoration, sie sind aus drei Metern noch lesbar:",
        "The lights around the rim are not decoration — they are still readable from three metres away:"),
        S_BODY))
    st += bullets([
        T("<b>Wie viele leuchten</b>, zeigt die <b>Restzeit</b> — der Kreis leert sich, während die Zeit läuft.",
          "<b>How many are lit</b> shows the <b>time remaining</b> — the circle empties as the time runs down."),
        T("<b>Die Farbe</b> sagt, <b>welche</b> Zeit gemeint ist: dieselbe, die auch auf dem Bildschirm zu sehen ist.",
          "<b>The colour</b> says <b>which</b> time is meant: the same one shown on the screen."),
        T("In den <b>letzten zehn Sekunden</b> pulsiert der Ring.",
          "Over the <b>last ten seconds</b> the ring pulses."),
        T("Beim <b>Alarm</b> blinkt er im Takt des Bildschirms.",
          "During an <b>alarm</b> it blinks in time with the screen."),
        T("Läuft gerade die <b>Stoppuhr</b>, wandert ein einzelner heller Punkt im Sekundentakt herum.",
          "While the <b>stopwatch</b> runs, a single bright dot travels around once per second."),
    ])

    st.append(Paragraph(T("Kleine Hilfe", "A little help"), S_H1))
    faq = [
        (T("Der Bildschirm ist dunkel.", "The screen is dark."),
         T("Nach einer Minute ohne Berührung schaltet er sich aus, um nicht zu stören. Einmal antippen "
           "oder am Ring drehen weckt ihn. <b>Die Zeiten laufen dabei weiter</b> — und wenn eine abläuft, "
           "geht der Bildschirm von selbst wieder an.",
           "After a minute without a touch it switches off so as not to be a nuisance. One tap, or a "
           "turn of the ring, wakes it. <b>Your times keep running</b> — and when one finishes, the "
           "screen comes back on by itself.")),
        (T("Ich habe den Stecker gezogen, und die Zeiten sind weg.",
           "I unplugged it and my times are gone."),
         T("Laufende Zeiten überstehen einen Stromausfall nicht — das Gerät hat keine Uhr, die "
           "weiterläuft. Ihre <b>Vorlagen, Helligkeit, Lautstärke und Sprache</b> bleiben dagegen gespeichert.",
           "Running times do not survive a loss of power — the device has no clock that keeps going. "
           "Your <b>presets, brightness, volume and language</b>, on the other hand, are kept.")),
        (T("Ich habe den Alarm überhört.", "I did not hear the alarm."),
         T("Lautstärke in den Einstellungen höher stellen. Das Brummen und der Leuchtring laufen "
           "unabhängig davon immer mit.",
           "Turn the volume up in the settings. The buzzing and the ring of lights always run "
           "regardless of it.")),
        (T("Ich habe aus Versehen etwas verstellt.", "I changed something by accident."),
         T("Der Ring ändert immer nur das, was gerade gelb umrandet ist. Tippen Sie einfach auf das, "
           "was Sie eigentlich meinten, und drehen Sie erneut.",
           "The ring only ever changes whatever currently has the yellow outline. Just tap the thing "
           "you actually meant and turn again.")),
        (T("Beim Löschen kommt eine Rückfrage.", "Deleting asks me to confirm."),
         T("Das ist Absicht: Löschen ist der einzige Handgriff, den man nicht zurücknehmen kann.",
           "That is deliberate: deleting is the one action that cannot be undone.")),
        (T("Die MEATER-App findet meinen Fühler nicht mehr.",
           "The MEATER app can no longer find my probe."),
         T("Dann ist RONDO damit verbunden — der Fühler lässt immer nur ein Gerät zu. Brauchen "
           "Sie die App doch einmal, trennen Sie RONDO kurz vom Strom.",
           "Then RONDO is connected to it — the probe only ever admits one device. If you do need "
           "the app, briefly disconnect RONDO from power.")),
        (T("Der Temperatur-Alarm hat nicht ausgelöst.", "The temperature alarm did not go off."),
         T("Er muss scharf sein — im Thermometer-Screen steht dann <b>Alarm aus</b> auf dem Knopf "
           "(das ist der Knopf zum Ausschalten) und der Zielkasten ist orange umrandet. Er löst "
           "einmal aus; verstellen Sie danach das Ziel, ist er wieder bereit.",
           "It has to be armed — on the thermometer page the button then reads <b>Alarm off</b> "
           "(that being the button that switches it off) and the target box has an orange outline. "
           "It fires once; change the target afterwards and it is ready again.")),
        (T("Das Thermometer zeigt nur das MEATER-Zeichen.",
           "The thermometer only shows the MEATER mark."),
         T("Der Fühler sendet nur, wenn er <b>aus der Ladeschale</b> genommen ist. Steckt er drin, "
           "bleibt es bei „suche“. Ausserdem sollte die Ladeschale in der Nähe stehen — sie ist "
           "der Verstärker für das Funksignal.",
           "The probe only transmits while it is <b>out of the charging case</b>. Left inside, the "
           "page stays on “searching”. The charging case should also be nearby — it is the repeater "
           "for the radio signal.")),
        (T("Das Gerät reagiert gar nicht mehr.", "The device has stopped responding entirely."),
         T("Kurz vom Strom trennen und wieder anstecken. Vorlagen und Einstellungen bleiben erhalten.",
           "Disconnect it from power briefly and plug it back in. Presets and settings are kept.")),
    ]
    for q, a in faq:
        # zusammenhalten - sonst steht die Frage allein am Seitenfuss
        st.append(KeepTogether([Paragraph(q, S_H2), Paragraph(a, S_BODY)]))

    st.append(Paragraph(T("Gut zu wissen", "Worth knowing"), S_H1))
    st += bullets([
        T("Bis zu <b>acht Zeiten gleichzeitig</b> und <b>zwanzig Vorlagen</b>.",
          "Up to <b>eight times at once</b> and <b>twenty presets</b>."),
        T("Eine gestartete Zeit wird automatisch zur Vorlage. Gleiche Zeit mit gleichem Bild "
          "überschreibt die vorhandene, statt eine zweite anzulegen.",
          "A time you start automatically becomes a preset. The same duration with the same picture "
          "overwrites the existing one instead of adding a second."),
        T("Kein WLAN, kein Konto, keine Cloud — nichts verlässt die Küche. Bluetooth benutzt "
          "RONDO nur, um dem Grill-Thermometer zuzuhören.",
          "No Wi-Fi, no account, no cloud — nothing leaves the kitchen. RONDO uses Bluetooth "
          "for one thing only: listening to the grill thermometer."),
        T("Der Ring lässt sich nicht drücken — das ist keine Fehlfunktion, sondern Bauart.",
          "The ring cannot be pressed — that is not a fault, it is how it is built."),
    ])
    return st

if __name__ == "__main__":
    doc, story = build()
    doc.build(content(story))
    print("geschrieben:", OUT, os.path.getsize(OUT) // 1024, "KB")
