/*
 * Minimal-Firmware fuer den ZWEITEN Chip (ESP32-U4WDH) des Knob-Boards.
 *
 * AKTUELL NICHT GEBRAUCHT: der Kuechentimer alarmiert optisch und haptisch, das
 * Board hat ohnehin keinen Lautsprecher. Bleibt liegen fuer den Fall, dass eine
 * Aktivbox an die Klinke soll - dann ist das hier die Voraussetzung.
 *
 * Wozu: Das Soft-Mute des PCM5100A (XSMT) haengt an IO32 dieses Chips und ist
 * vom ESP32-S3 aus nicht erreichbar. Ohne HIGH dort bleibt der DAC stumm -
 * egal wie korrekt das I2S auf dem S3 konfiguriert ist.
 *
 * Flashen:
 *   1. USB-A-auf-C-Kabel verwenden (bei C-auf-C entscheidet der Host-Mux und es
 *      ist nicht deterministisch, welcher Chip am Port haengt).
 *   2. Klappt der Upload nicht: Kabel um 180 Grad drehen - dann haengt der
 *      andere Chip dran.
 *   3. Arduino IDE, Board "ESP32 Dev Module" (NICHT S3), Upload.
 *
 * Danach nie wieder anfassen. Der I2S-Umschalter (CH445P) wird hier bewusst
 * NICHT bedient - den steuert der S3 ueber seinen GPIO0.
 */
#define XSMT 32

void setup() {
  pinMode(XSMT, OUTPUT);
  digitalWrite(XSMT, HIGH);   // DAC entstummen und so lassen
}

void loop() {
  digitalWrite(XSMT, HIGH);   // falls der Pin je zurueckfaellt
  delay(1000);
}
