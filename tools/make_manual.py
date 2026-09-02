#!/usr/bin/env python3
"""Erzeugt die Bedienungsanleitung als PDF.

Die Screenshots stammen aus dem Host-Simulator (tools/sim), sind also
tatsaechlich das, was auf dem Geraet zu sehen ist - keine Zeichnungen.
Vorher einmal ./tools/sim/build.sh laufen lassen.

    python3 tools/make_manual.py            -> RONDO-Anleitung.pdf
"""
import os, math
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

OUT   = "RONDO-Anleitung.pdf"
SHOTS = "tools/sim/out"
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

    centred("Küchentimer", 14, H * 0.24, CREAM)
    centred("Bedienungsanleitung", 10.5, H * 0.24 - 20, colors.HexColor("#9A9A9A"))
    c.restoreState()

def content_page(c, doc):
    c.saveState()
    c.setFont(FONT, 8)
    c.setFillColor(colors.HexColor("#9A9A9A"))
    c.drawString(MARGIN, H - 12 * mm, "RONDO  ·  Küchentimer")
    c.setStrokeColor(RULE)
    c.setLineWidth(0.5)
    c.line(MARGIN, H - 14 * mm, W - MARGIN, H - 14 * mm)
    c.drawRightString(W - MARGIN, 12 * mm, str(doc.page - 1))
    # kleines Logo als Fusszeichen
    draw_logo(c, MARGIN + 3, 12 * mm + 3, 0, 1.6, highlight=-1)
    c.restoreState()

# ---------------------------------------------------------------- Bausteine
def shot(name, width=47 * mm):
    return Image(os.path.join(SHOTS, name), width=width, height=width)

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
                          title="RONDO Küchentimer – Bedienungsanleitung",
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
    st.append(Paragraph("Willkommen", S_H1))
    st.append(Paragraph(
        "RONDO ist eine Küchenuhr. Sie stellen eine Zeit ein, und wenn sie abgelaufen ist, "
        "meldet sich das Gerät — hörbar, sichtbar und spürbar. Mehrere Zeiten gleichzeitig sind "
        "kein Problem: Kartoffeln, Ei und Braten laufen nebeneinander, jedes in einer eigenen Farbe.", S_BODY))
    st.append(Paragraph(
        "Es braucht kein Internet, kein Konto und keine App. Alles passiert auf dem Gerät selbst. "
        "Sie können es also einfach einstecken und benutzen.", S_BODY))

    st.append(Paragraph("Das Gerät", S_H2))
    st += bullets([
        "<b>Der runde Bildschirm</b> zeigt alles an — und reagiert auf Berührung, wie ein Handy.",
        "<b>Der Ring aussen herum lässt sich drehen.</b> Er rastet spürbar ein: ein Klick, ein Schritt. "
        "Drücken lässt er sich nicht — alles, was gedrückt werden will, tippen Sie auf dem Bildschirm an.",
        "<b>Hinter dem Rand leuchten kleine Lämpchen</b> im Kreis. Sie zeigen aus der Ferne, "
        "wie viel Zeit noch übrig ist.",
        "<b>Ein Lautsprecher</b> für die Melodie und <b>ein kleiner Motor</b>, der das Gerät leise "
        "brummen lässt — so merken Sie den Alarm auch, wenn es laut ist in der Küche.",
    ])

    st.append(Paragraph("Die drei Handgriffe", S_H1))
    st.append(Paragraph("Mehr müssen Sie sich nicht merken. Diese drei gelten überall gleich:", S_BODY))
    t = Table([
        [Paragraph("<b>Wischen</b><br/>nach links oder rechts", S_BODY),
         Paragraph("wechselt die <b>Seite</b>. Die Punkte unten am Rand zeigen, auf welcher Sie gerade sind.", S_BODY)],
        [Paragraph("<b>Ring drehen</b>", S_BODY),
         Paragraph("ändert <b>etwas auf der Seite</b>, auf der Sie sind — eine Zahl, eine Auswahl, "
                   "oder blättert durch eine Liste. Die Seite wechselt der Ring nie.", S_BODY)],
        [Paragraph("<b>Tippen</b>", S_BODY),
         Paragraph("wählt aus und bestätigt. Was Sie angetippt haben, wird gelb umrandet — "
                   "das ist dann das, was der Ring verstellt.", S_BODY)],
    ], colWidths=[45 * mm, None])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"),
                           ("LEFTPADDING", (0, 0), (0, -1), 0),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
                           ("LINEBELOW", (0, 0), (-1, -2), 0.4, RULE)]))
    st.append(t)
    st.append(Paragraph(
        "Wenn Sie sich verlaufen: einfach so lange wischen, bis Sie wieder auf einer bekannten Seite sind. "
        "Kaputtmachen können Sie nichts.", S_NOTE))

    st.append(Paragraph("Die acht Seiten", S_H1))
    st.append(Paragraph(
        "Die Seiten liegen nebeneinander wie Bilder in einem Album. Nach dem Einschalten stehen Sie "
        "auf <b>Läuft gerade</b>. Von dort nach rechts wischen führt zur Übersicht, nach links zu allem anderen.", S_BODY))
    seiten = [
        ("Übersicht", "alle laufenden Zeiten auf einen Blick"),
        ("Läuft gerade", "die Startseite: eine Zeit gross, mit den Knöpfen dazu"),
        ("Neuer Timer", "hier stellen Sie eine neue Zeit ein"),
        ("Eieruhr", "rechnet die Kochzeit für Eier aus"),
        ("Grill-Thermometer", "zeigt die Temperatur im Fleisch"),
        ("Stoppuhr", "zählt vorwärts statt rückwärts"),
        ("Vorlagen", "gespeicherte Zeiten, mit einem Tipp gestartet"),
        ("Einstellungen", "Helligkeit, Lautstärke, Akku"),
    ]
    rows = [[Paragraph(f"<b>{n}</b>", S_BODY), Paragraph(b, S_BODY)] for n, b in seiten]
    t = Table(rows, colWidths=[38 * mm, None])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"),
                           ("LEFTPADDING", (0, 0), (0, -1), 0),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
    st.append(t)

    st.append(Paragraph("Ihr erster Timer", S_H1))
    st.append(screen_block(
        shot("3-neuer-timer.png"), "So stellen Sie fünf Minuten ein",
        ["Wischen Sie von der Startseite <b>zweimal nach links</b>, bis oben „Neuer Timer“ steht."],
        "Die Seite „Neuer Timer“",
        extra=[
            Paragraph("1.&nbsp;&nbsp;Tippen Sie auf die <b>mittleren zwei Ziffern</b> — das sind die "
                      "Minuten. Sie werden gelb.", S_STEP),
            Paragraph("2.&nbsp;&nbsp;Drehen Sie den Ring, bis <b>05</b> dasteht. Ein Klick ist eine Minute.", S_STEP),
            Paragraph("3.&nbsp;&nbsp;Tippen Sie auf <b>Start</b>. Fertig.", S_STEP),
        ]))
    st.append(Paragraph(
        "Noch schneller geht es mit den vier Knöpfen darunter: <b>3, 5, 10 und 15 Minuten</b> mit einem Tipp.", S_BODY))
    st.append(Paragraph(
        "Die beiden Knöpfe in der Mitte sind Geschmackssache: links wählen Sie die <b>Melodie</b>, die am Ende "
        "spielt (sie wird beim Auswählen gleich vorgespielt), rechts ein <b>Bild</b> — Ei, Pasta, Brot und so "
        "weiter. Das Bild hilft später, die Timer auseinanderzuhalten.", S_BODY))

    st.append(Paragraph("Wenn die Zeit um ist", S_H1))
    st.append(screen_block(
        shot("6-alarm.png"), "Der Alarm",
        ["Das Gerät holt die abgelaufene Zeit von selbst nach vorne, egal wo Sie gerade waren. "
         "Der Bildschirm blinkt in der Farbe des Timers, das Gerät brummt, die Melodie spielt "
         "und die Lämpchen blinken mit.",
         "<b>Zum Ausschalten tippen Sie einfach mitten auf den Bildschirm.</b> Die ganze Fläche ist "
         "die Taste — das klappt auch mit dem Handrücken, wenn die Finger teigig sind.",
         "Brauchen Sie doch noch etwas Zeit, tippen Sie auf <b>+5 Min</b>."],
        "Ein abgelaufener Timer"))
    st.append(Paragraph(
        "Wenn niemand reagiert, hört der Alarm nach fünf Minuten von selbst auf. Der Timer bleibt "
        "dann als „abgelaufen“ stehen, damit Sie sehen, dass etwas fertig geworden ist.", S_NOTE))

    st.append(Paragraph("Mehrere Zeiten gleichzeitig", S_H1))
    st.append(screen_block(
        shot("2-aktiv-drei.png"), "Läuft gerade",
        ["Hier steht immer <b>eine</b> Zeit gross im Bild. Der farbige Bogen aussen herum leert sich, "
         "während die Zeit abläuft — so sehen Sie den Fortschritt, ohne die Zahl zu lesen.",
         "Laufen mehrere Zeiten, steht unter den Knöpfen zum Beispiel <b>1 / 3</b>. Mit dem <b>Ring</b> "
         "oder mit Wischen nach oben und unten blättern Sie durch.",
         "<b>Jede Zeit hat ihre eigene Farbe.</b> Der Leuchtring am Gerät nimmt immer die Farbe der Zeit an, "
         "die Sie gerade ansehen.",
         "Läuft gerade nichts, zeigt RONDO sein Zeichen: der orange Punkt wandert langsam im Kreis. "
         "Ein Tipp darauf führt direkt zum neuen Timer."],
        "Drei Timer, hier der zweite"))
    st.append(screen_block(
        shot("2b-uebersicht.png"), "Übersicht",
        ["Eine Seite nach rechts liegt die Liste aller laufenden Zeiten. Ein Tipp auf eine Zeile "
         "holt sie gross nach vorne.",
         "Praktisch, wenn drei Sachen gleichzeitig kochen und Sie nur kurz schauen wollen, "
         "was als Nächstes fertig wird."],
        "Alles auf einen Blick"))

    st.append(Paragraph("Die anderen Seiten", S_H1))
    st.append(screen_block(
        shot("3c-eieruhr.png"), "Eieruhr",
        ["Sagen Sie dem Gerät, <b>wie gross</b> das Ei ist, ob es <b>aus dem Kühlschrank</b> kommt "
         "und wie Sie es <b>mögen</b> — weich, wachsweich oder hart. Die Kochzeit rechnet es aus.",
         "Zeile antippen, Ring drehen, <b>Kochen</b> tippen. Die Zeit läuft dann wie jede andere."],
        "Ein mittleres Ei aus dem Kühlschrank, weich"))
    st.append(screen_block(
        shot("3e-fuehler.png"), "Grill-Thermometer",
        ["Haben Sie ein <b>MEATER</b>-Thermometer, zeigt RONDO an, wie warm es im Fleisch ist — "
         "gross die <b>Kerntemperatur</b>, darunter die Temperatur im Garraum und der Ladestand "
         "des Fühlers.",
         "Sie müssen nichts einrichten: <b>Fühler aus der Ladeschale nehmen</b>, und RONDO "
         "verbindet sich von selbst. Der Punkt unten wird grün, sobald Werte ankommen.",
         "Der Fühler spricht immer nur mit einem Gerät — und das ist hier RONDO. Die "
         "Hersteller-App bleibt aussen vor, solange das Thermometer verbunden ist. So ist es "
         "gedacht: eine Zahl, ein Alarm, kein Assistent."],
        "Ein Braten bei 62,4 °C",
        extra=[
            Paragraph("<b>Damit das Fleisch Sie ruft</b>", S_H2),
            Paragraph("Drehen Sie am Ring, bis die gewünschte <b>Zieltemperatur</b> im Kasten steht — "
                      "rechts daneben steht, was das bedeutet: <i>englisch</i>, <i>rosa</i>, "
                      "<i>medium</i>, <i>durch</i>. Dann auf <b>Alarm ein</b> tippen.", S_BODY),
            Paragraph("Ist die Temperatur erreicht, meldet sich RONDO genau wie bei einer "
                      "abgelaufenen Zeit: der Bildschirm blinkt, das Gerät brummt, die Melodie "
                      "spielt. <b>Stopp</b> beendet es.", S_BODY),
        ]))
    st.append(screen_block(
        shot("3d-stoppuhr.png"), "Stoppuhr",
        ["Zählt vorwärts statt rückwärts — für „wie lange köchelt das jetzt schon?“.",
         "Sie läuft im Hintergrund weiter, auch wenn Sie wegwischen. Solange sie läuft, taucht sie "
         "auch auf der Seite „Läuft gerade“ und in der Übersicht auf."],
        "Start, Pause, Zurück"))
    st.append(screen_block(
        shot("4-vorlagen.png"), "Vorlagen",
        ["Jede Zeit, die Sie starten, merkt sich das Gerät automatisch. Hier stehen sie alle.",
         "<b>Antippen startet</b> die Zeit sofort. <b>Länger draufhalten</b> öffnet ein kleines Menü "
         "zum Ändern oder Löschen."],
        "Gespeicherte Zeiten"))
    st.append(screen_block(
        shot("5-einstellungen.png"), "Einstellungen",
        ["<b>Helligkeit</b> und <b>Lautstärke</b>: Zeile antippen, dann mit dem Ring einstellen.",
         "<b>Alarm testen</b> spielt einmal vor, was passiert, wenn eine Zeit abläuft.",
         "Ganz unten steht der <b>Akkustand</b>. Hängt das Gerät am Strom, zeigt es die Spannung "
         "statt einer Prozentzahl — beim Laden wäre eine Prozentangabe geraten."],
        "Helligkeit und Lautstärke"))

    st.append(Paragraph("Der Leuchtring", S_H1))
    st.append(Paragraph(
        "Die Lämpchen am Rand sind nicht Dekoration, sie sind aus drei Metern noch lesbar:", S_BODY))
    st += bullets([
        "<b>Wie viele leuchten</b>, zeigt die <b>Restzeit</b> — der Kreis leert sich, während die Zeit läuft.",
        "<b>Die Farbe</b> sagt, <b>welche</b> Zeit gemeint ist: dieselbe, die auch auf dem Bildschirm zu sehen ist.",
        "In den <b>letzten zehn Sekunden</b> pulsiert der Ring.",
        "Beim <b>Alarm</b> blinkt er im Takt des Bildschirms.",
        "Läuft gerade die <b>Stoppuhr</b>, wandert ein einzelner heller Punkt im Sekundentakt herum.",
    ])

    st.append(Paragraph("Kleine Hilfe", S_H1))
    faq = [
        ("Der Bildschirm ist dunkel.",
         "Nach einer Minute ohne Berührung schaltet er sich aus, um nicht zu stören. Einmal antippen "
         "oder am Ring drehen weckt ihn. <b>Die Zeiten laufen dabei weiter</b> — und wenn eine abläuft, "
         "geht der Bildschirm von selbst wieder an."),
        ("Ich habe den Stecker gezogen, und die Zeiten sind weg.",
         "Laufende Zeiten überstehen einen Stromausfall nicht — das Gerät hat keine Uhr, die "
         "weiterläuft. Ihre <b>Vorlagen, Helligkeit und Lautstärke</b> bleiben dagegen gespeichert."),
        ("Ich habe den Alarm überhört.",
         "Lautstärke in den Einstellungen höher stellen. Das Brummen und der Leuchtring laufen "
         "unabhängig davon immer mit."),
        ("Ich habe aus Versehen etwas verstellt.",
         "Der Ring ändert immer nur das, was gerade gelb umrandet ist. Tippen Sie einfach auf das, "
         "was Sie eigentlich meinten, und drehen Sie erneut."),
        ("Beim Löschen kommt eine Rückfrage.",
         "Das ist Absicht: Löschen ist der einzige Handgriff, den man nicht zurücknehmen kann."),
        ("Die MEATER-App findet meinen Fühler nicht mehr.",
         "Dann ist RONDO damit verbunden — der Fühler lässt immer nur ein Gerät zu. Brauchen "
         "Sie die App doch einmal, trennen Sie RONDO kurz vom Strom."),
        ("Der Temperatur-Alarm hat nicht ausgelöst.",
         "Er muss scharf sein — im Thermometer-Screen steht dann <b>Alarm aus</b> auf dem Knopf "
         "(das ist der Knopf zum Ausschalten) und der Zielkasten ist orange umrandet. Er löst "
         "einmal aus; verstellen Sie danach das Ziel, ist er wieder bereit."),
        ("Das Thermometer zeigt nur Striche.",
         "Der Fühler sendet nur, wenn er <b>aus der Ladeschale</b> genommen ist. Steckt er drin, "
         "bleibt es bei „suche“. Ausserdem sollte die Ladeschale in der Nähe stehen — sie ist "
         "der Verstärker für das Funksignal."),
        ("Das Gerät reagiert gar nicht mehr.",
         "Kurz vom Strom trennen und wieder anstecken. Vorlagen und Einstellungen bleiben erhalten."),
    ]
    for q, a in faq:
        st.append(Paragraph(q, S_H2))
        st.append(Paragraph(a, S_BODY))

    st.append(Paragraph("Gut zu wissen", S_H1))
    st += bullets([
        "Bis zu <b>acht Zeiten gleichzeitig</b> und <b>zwanzig Vorlagen</b>.",
        "Eine gestartete Zeit wird automatisch zur Vorlage. Gleiche Zeit mit gleichem Bild "
        "überschreibt die vorhandene, statt eine zweite anzulegen.",
        "Das Gerät funkt nicht. Nichts verlässt die Küche.",
        "Der Ring lässt sich nicht drücken — das ist keine Fehlfunktion, sondern Bauart.",
    ])
    return st

if __name__ == "__main__":
    doc, story = build()
    doc.build(content(story))
    print("geschrieben:", OUT, os.path.getsize(OUT) // 1024, "KB")
