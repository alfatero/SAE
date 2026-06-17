#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/* ==== À PERSONNALISER ==== */
const char* WIFI_SSID     = "Sa";
const char* WIFI_PASSWORD = "18042005";

const char* MQTT_HOST     = "172.20.10.3";  // Broker privé
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_USER     = "sacha";
const char* MQTT_PASS     = "sacha";

const char* TOPIC_PRISE   = "/prise1";   // ON / OFF (retained)
/* ======================== */

// Broches (selon ton montage)
const int PIN_BOUTON = 13;   // GPIO13 -> bouton vers GND
const int PIN_LED    = 15;   // GPIO15 -> LED -> résistance -> GND

WiFiClient espClient;
PubSubClient mqtt(espClient);

// Etat de la prise/LED
bool priseOn = false;

// Anti-rebond
bool lastBtnRaw = HIGH;                 // avec INPUT_PULLUP : HIGH = repos
bool lastBtnStable = HIGH;
unsigned long lastChangeMs = 0;
const unsigned long DEBOUNCE_MS = 50;

// Reconnexions non bloquantes
unsigned long lastWifiAttemptMs = 0;
unsigned long lastMqttAttemptMs = 0;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long MQTT_RETRY_MS = 3000;

bool handlingMqttMessage = false;

void applyOutput(bool on) {
  digitalWrite(PIN_LED, on ? HIGH : LOW);  // si LED active LOW, inverser ici
}

void publishState(const char* source = "local") {
  if (!mqtt.connected()) return;          // publie seulement si connecté
  if (handlingMqttMessage) return;        // évite l'écho
  const char* payload = priseOn ? "ON" : "OFF";
  bool ok = mqtt.publish(TOPIC_PRISE, payload, true); // retained
  Serial.print("[PUB]["); Serial.print(source); Serial.print("] ");
  Serial.print(payload); Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length+1);
  for (unsigned int i=0;i<length;i++) msg += (char)payload[i];
  msg.trim(); msg.toUpperCase();

  Serial.print("[MQTT] "); Serial.print(topic); Serial.print(" <= "); Serial.println(msg);

  if (String(topic) == TOPIC_PRISE) {
    bool newState;
    if (msg == "ON") newState = true;
    else if (msg == "OFF") newState = false;
    else { Serial.println("[WARN] payload attendu: ON/OFF"); return; }

    if (newState != priseOn) {
      handlingMqttMessage = true;
      priseOn = newState;
      applyOutput(priseOn);
      handlingMqttMessage = false;
      // pas de republication ici pour éviter boucle
    }
  }
}

void tryWifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastWifiAttemptMs < WIFI_RETRY_MS) return;
  lastWifiAttemptMs = now;

  Serial.print("WiFi -> connexion à "); Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void tryMqttConnect() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  unsigned long now = millis();
  if (now - lastMqttAttemptMs < MQTT_RETRY_MS) return;
  lastMqttAttemptMs = now;

  String clientId = String("ESP8266-PRISE-") + String(ESP.getChipId(), HEX);
  Serial.print("MQTT -> ");
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
    Serial.println("connecté");
    mqtt.subscribe(TOPIC_PRISE);
    Serial.print("Abonné: "); Serial.println(TOPIC_PRISE);
    publishState("boot");
  } else {
    Serial.print("échec (rc="); Serial.print(mqtt.state()); Serial.println(")");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOUTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  applyOutput(priseOn);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);

  // Lancement des tentatives (non bloquant)
  tryWifiConnect();
}

void loop() {
  // Connexions non bloquantes
  if (WiFi.status() != WL_CONNECTED) {
    tryWifiConnect();
  } else {
    if (!mqtt.connected()) tryMqttConnect();
    mqtt.loop(); // tournant rapide
  }

  // Lecture bouton + anti-rebond
  bool btnRaw = digitalRead(PIN_BOUTON);
  unsigned long now = millis();

  if (btnRaw != lastBtnRaw) {
    lastChangeMs = now;
    lastBtnRaw = btnRaw;
  }
  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    // front descendant stable (HIGH -> LOW)
    if (lastBtnStable == HIGH && lastBtnRaw == LOW) {
      priseOn = !priseOn;              // toggle
      applyOutput(priseOn);
      publishState("button");
      Serial.print("[BTN] toggle -> "); Serial.println(priseOn ? "ON" : "OFF");
    }
    lastBtnStable = lastBtnRaw;
  }
}










