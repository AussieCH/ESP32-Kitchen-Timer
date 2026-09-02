// MEATER-Fleischfuehler ueber Bluetooth Low Energy.
//
// Kennungen und Umrechnung stammen aus der offengelegten Arbeit der Community
// (nathanfaber/meaterble und die ESPHome-Umsetzung von MortenVinding), am
// Geraet gegengeprueft. Der Fuehler sendet von sich aus (notify), es braucht
// keine Kopplung und kein Pairing.
//
// Zwei Eigenheiten des Fuehlers, die man kennen muss:
//   * Er laesst nur eine einzige Verbindung zu. Ist die App verbunden, kommen
//     wir nicht dran - und andersherum.
//   * Er sendet nur, wenn er aus der Ladeschale genommen wurde.
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "meater.h"

#define MEATER_SVC   "a75cc7fc-c956-488f-ac2a-2dbc08b63a04"
#define MEATER_TEMP  "7edda774-045e-4bbf-909b-45d1991a2876"
#define MEATER_BATT  "2adb4877-68d8-4884-bd3c-d83853bf27b8"

static volatile MeaterState s_state = MEATER_OFF;
static volatile float s_tip = 0, s_amb = 0;
static volatile int   s_batt = -1;
static volatile uint32_t s_last_ms = 0;
static char s_name[24] = "";

static NimBLEClient *s_client = nullptr;
static NimBLEAddress s_addr;
static volatile bool s_found = false;
static volatile bool s_want_connect = false;

// ---------------------------------------------------------------- Messwerte
static void decode_temp(const uint8_t *d, size_t len) {
  if (len < 8) return;
  int tip = d[0] + (d[1] << 8);
  int ra  = d[2] + (d[3] << 8);
  int oa  = d[4] + (d[5] << 8);
  s_tip = (tip + 8.0f) / 16.0f;
  int corr = ((ra - min(48, oa)) * 16 * 589) / 1487;
  s_amb = (tip + max(0, corr) + 8.0f) / 16.0f;
  s_last_ms = millis();
}

static void notify_cb(NimBLERemoteCharacteristic *, uint8_t *data, size_t len, bool) {
  decode_temp(data, len);
  static uint32_t last_log = 0;
  if (Serial && millis() - last_log > 3000) {      // nur alle drei Sekunden
    last_log = millis();
    Serial.printf("[meater] Kern %.1f °C   Garraum %.1f °C   (%u Bytes)\n",
                  s_tip, s_amb, (unsigned)len);
  }
}

// ---------------------------------------------------------------- Callbacks
class ScanCB : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice *dev) override {
    bool match = dev->isAdvertisingService(NimBLEUUID(MEATER_SVC));
    if (!match && dev->haveName()) {
      String n = dev->getName().c_str();
      n.toUpperCase();
      match = n.startsWith("MEATER");
    }
    if (!match) return;
    s_addr = dev->getAddress();
    strncpy(s_name, dev->haveName() ? dev->getName().c_str() : "MEATER", sizeof(s_name) - 1);
    s_found = true;
    NimBLEDevice::getScan()->stop();
  }
};

class ClientCB : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient *, int reason) override {
    Serial.printf("[meater] Verbindung weg (%d)\n", reason);
    s_state = MEATER_SEARCHING;
    s_batt = -1;
    s_want_connect = false;
  }
};

static ScanCB   s_scan_cb;
static ClientCB s_client_cb;

// ---------------------------------------------------------------- Verbinden
static bool connect_probe() {
  if (!s_client) {
    s_client = NimBLEDevice::createClient();
    s_client->setClientCallbacks(&s_client_cb, false);
    s_client->setConnectTimeout(8000);
  }
  if (!s_client->connect(s_addr)) {
    Serial.println("[meater] Verbindung fehlgeschlagen");
    return false;
  }

  // Einmal auflisten, was der Fuehler anbietet - so sieht man sofort, ob ein
  // anderes Modell andere Kennungen benutzt, statt im Dunkeln zu suchen.
  for (auto *svc : s_client->getServices(true)) {
    Serial.printf("[meater] Dienst %s\n", svc->getUUID().toString().c_str());
    for (auto *ch : svc->getCharacteristics(true))
      Serial.printf("[meater]    Merkmal %s%s\n", ch->getUUID().toString().c_str(),
                    ch->canNotify() ? "  (sendet)" : "");
  }

  NimBLERemoteService *svc = s_client->getService(MEATER_SVC);
  if (!svc) { Serial.println("[meater] Dienst nicht gefunden - anderes Modell?"); s_client->disconnect(); return false; }

  NimBLERemoteCharacteristic *temp = svc->getCharacteristic(MEATER_TEMP);
  if (!temp || !temp->subscribe(true, notify_cb)) {
    Serial.println("[meater] Temperaturmerkmal nicht abonnierbar");
    s_client->disconnect();
    return false;
  }

  NimBLERemoteCharacteristic *bat = svc->getCharacteristic(MEATER_BATT);
  if (bat && bat->canRead()) {
    NimBLEAttValue v = bat->readValue();
    if (v.size() >= 2) s_batt = (v[0] + v[1]) * 10;
  }

  s_state = MEATER_CONNECTED;
  Serial.printf("[meater] verbunden mit %s\n", s_name);
  return true;
}

// ---------------------------------------------------------------- Task
static void meater_task(void *) {
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(&s_scan_cb, false);
  scan->setActiveScan(true);
  scan->setInterval(200);
  scan->setWindow(60);          // kurzes Fenster: der Funk teilt sich den Chip
                                // mit dem Display, dauerhaftes Horchen ruckelt

  for (;;) {
    if (s_state != MEATER_CONNECTED) {
      if (!s_found) {
        s_state = MEATER_SEARCHING;
        scan->start(4000, false, true);      // blockiert diesen Task, nicht das UI
        vTaskDelay(pdMS_TO_TICKS(1500));
      } else {
        s_found = false;
        if (!connect_probe()) vTaskDelay(pdMS_TO_TICKS(3000));
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

void meater_init() {
  NimBLEDevice::init("RONDO");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  s_state = MEATER_SEARCHING;
  xTaskCreatePinnedToCore(meater_task, "meater", 5120, nullptr, 1, nullptr, 0);
  Serial.println("[meater] Suche laeuft");
}

MeaterState meater_state()  { return s_state; }
float meater_tip_c()        { return s_tip; }
float meater_ambient_c()    { return s_amb; }
int   meater_battery()      { return s_batt; }
uint32_t meater_age_ms()    { return s_last_ms ? millis() - s_last_ms : 0xFFFFFFFF; }
const char *meater_name()   { return s_name[0] ? s_name : "MEATER"; }
