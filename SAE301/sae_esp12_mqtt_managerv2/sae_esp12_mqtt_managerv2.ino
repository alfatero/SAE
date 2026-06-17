#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>  // tzapu/WiFiManager

/* ==== PARAMÈTRES MQTT ==== */
const char* MQTT_HOST     = "172.20.10.3";  // IP broker
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_USER     = "sacha";
const char* MQTT_PASS     = "sacha";

const char* TOPIC_STATE   = "/plugs/1/state";   // commande + état (1 seul topic)
/* ========================= */

// Broches
const int PIN_BOUTON = 13;   // GPIO13 -> bouton vers GND
const int PIN_LED    = 15;   // GPIO15 -> LED -> résistance -> GND

WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiManager wm;

// Etat de la prise/LED
bool priseOn = false;

// Anti-rebond
bool lastBtnRaw = HIGH;
bool lastBtnStable = HIGH;
unsigned long lastChangeMs = 0;
const unsigned long DEBOUNCE_MS = 50;

// Reconnexions non bloquantes
unsigned long lastMqttAttemptMs = 0;
const unsigned long MQTT_RETRY_MS = 3000;

void applyOutput(bool on) {
  // ⚠️ Inverse si ta LED est active LOW
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

void publishState(const char* source = "local") {
  if (!mqtt.connected()) return;
  const char* payload = priseOn ? "ON" : "OFF";
  bool ok = mqtt.publish(TOPIC_STATE, payload, true); // retained = true
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

  if (String(topic) == TOPIC_STATE) {
    bool newState;
    if (msg == "ON") newState = true;
    else if (msg == "OFF") newState = false;
    else { Serial.println("[WARN] payload attendu: ON/OFF"); return; }

    if (newState != priseOn) {
      priseOn = newState;
      applyOutput(priseOn);
      publishState("mqtt");   // republie l’état réel pour confirmation
    }
  }
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
    mqtt.subscribe(TOPIC_STATE);
    Serial.print("Abonné: "); Serial.println(TOPIC_STATE);
    publishState("boot");   // annonce l’état actuel au démarrage
  } else {
    Serial.print("échec (rc="); Serial.print(mqtt.state()); Serial.println(")");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOUTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  applyOutput(priseOn);

  // --- WiFiManager ---
  WiFi.mode(WIFI_STA);
  wm.setClass("invert");
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);
  bool ok = wm.autoConnect("PriseWifi-Setup");
  Serial.print("WiFiManager autoConnect: "); Serial.println(ok ? "OK" : "PORTAIL");

  // MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
}

void loop() {
  wm.process();

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) tryMqttConnect();
    mqtt.loop();
  }

  // Gestion bouton avec anti-rebond
  bool btnRaw = digitalRead(PIN_BOUTON);
  unsigned long now = millis();

  if (btnRaw != lastBtnRaw) {
    lastChangeMs = now;
    lastBtnRaw = btnRaw;
  }
  if ((now - lastChangeMs) > DEBOUNCE_MS) {
    if (lastBtnStable == HIGH && lastBtnRaw == LOW) {
      priseOn = !priseOn;
      applyOutput(priseOn);
      publishState("button");
      Serial.print("[BTN] toggle -> "); Serial.println(priseOn ? "ON" : "OFF");
    }
    lastBtnStable = lastBtnRaw;
  }
}


