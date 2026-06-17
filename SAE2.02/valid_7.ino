/*  ESP32 WebRadio + VS1053  – v3.0
 *  -------------------------------------------------------------
 *  - 7 présélections + 1 URL libre (topic radio/url)
 *  - Commandes MQTT : voir tableau plus bas
 *  - Volume modifiable par slider (dB) OU par boutons +- / mute
 *  - Fonctions STOP et REVENIR ajoutées
 *  - Publication systématique du statut pour garder l’IHM à jour
 */

#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP32_VS1053_Stream.h>
#include <SPI.h>

/*** Brochage VS1053 ***/
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;
WiFiClient          espClient;
PubSubClient        mqttClient(espClient);

/*** Topics MQTT ***/
const char* mqttServer   = "broker.mqtt-dashboard.com";
const int   mqttPort     = 1883;

const char* cmdTopic     = "radio/commande";     // commandes boutons / UI
const char* volTopic     = "radio/volume_dB";    // slider volume (en dB)
const char* urlTopic     = "radio/url";          // URL libre
const char* statusTopic  = "radio/statut";       // état complet (retained)
const char* bassTopic    = "radio/bass";
const char* trebleTopic  = "radio/treble";

/*** État lecteur ***/
int   volume      = 85;     // 0-100  (converti +1 = -0,5 dB)
int   volumePrev  = 85;     // pour mute / unmute
bool  muted       = false;
bool  playing     = false;  // sert au STOP

/*** Stations présélectionnées ***/
#define NB_PRESETS 7
const char* presets[NB_PRESETS] = {
  "http://stream03.ustream.ca:8000/cism128.mp3",
  "http://chisou-02.cdn.eurozet.pl:8112/;",
  "http://streamer01.sti.usherbrooke.ca:8000/cfak.mp3",
  "http://radios.rtbf.be/wr-c21-metal-128.mp3",
  "http://ecoutez.chyz.ca:8000/mp3",
  "http://www.skyrock.fm/stream.php/tunein16_128mp3.mp3",
  "http://lyon1ere.ice.infomaniak.ch/lyon1ere-high.mp3"
};
int     presetIndex   = 0;        // 0‥6
bool    customActive  = false;    // true si URL libre jouée
String  customURL     = "";

/*** Tonalité (grave/aigu) & spatialisation ***/
uint8_t trebleAmp = 5, trebleFreq = 2;
uint8_t bassAmp   = 5, bassFreq   = 15;
uint8_t tone[4]   = { trebleFreq, trebleAmp, bassFreq, bassAmp };
uint8_t spatial   = 0;            // 0 = OFF, 1 = LO, 2 = HI, 3 = LO+HI

/* ----------  OUTILS  ---------- */
void publishLevels()             // publie dB, bass et treble
{
  char buf[16];

  dtostrf(-0.5f * volume, 0, 1, buf);          // Slider (dB)
  mqttClient.publish(volTopic, buf, true);

  sprintf(buf, "%d", bassAmp);                 // Bass
  mqttClient.publish(bassTopic, buf, true);

  sprintf(buf, "%d", trebleAmp);               // Treble
  mqttClient.publish(trebleTopic, buf, true);
}

void publishStatus()             // statut global pour l’IHM
{
  char buf[256];
  if (customActive) {
    snprintf(buf, sizeof(buf),
      "station=CUSTOM,url=%s,volume=%d,treble=%d,bass=%d,spatial=%d,playing=%d",
      customURL.c_str(), volume, trebleAmp, bassAmp, spatial, playing);
  } else {
    snprintf(buf, sizeof(buf),
      "station=%d,volume=%d,treble=%d,bass=%d,spatial=%d,playing=%d",
      presetIndex, volume, trebleAmp, bassAmp, spatial, playing);
  }
  mqttClient.publish(statusTopic, buf, true);
}

void sendAll() { publishStatus(); publishLevels(); }

/* ----------  VS1053 : spatialisation ---------- */
uint16_t readSci(uint8_t reg)
{
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03); SPI.transfer(reg);
  uint16_t hi = SPI.transfer(0xFF), lo = SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return (hi << 8) | lo;
}
void writeSci(uint8_t reg, uint16_t data)
{
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02); SPI.transfer(reg);
  SPI.transfer(data >> 8); SPI.transfer(data & 0xFF);
  digitalWrite(VS1053_CS, HIGH);
}
void toggleSpatial()
{
  static const uint16_t SM_LO = (1 << 4), SM_HI = (1 << 7);
  uint16_t m = readSci(0);
  m &= ~(SM_LO | SM_HI);

  spatial = (spatial + 1) % 4;
  if (spatial & 1) m |= SM_LO;
  if (spatial & 2) m |= SM_HI;

  writeSci(0, m);
}

/* ----------  LECTURE STATION ---------- */
void startCurrentStream()
{
  audio.stopSong();
  delay(50);

  if (customActive)
    audio.connecttohost(customURL.c_str());
  else
    audio.connecttohost(presets[presetIndex]);

  delay(1500);
  audio.setVolume(volume);
  audio.setTone(tone);

  /* remise de la spatialisation */
  uint8_t s = spatial; spatial = 255; spatial = s; toggleSpatial();

  playing = true;
  sendAll();
}

/* ----------  CALLBACK MQTT ---------- */
void mqttCallback(char* topic, byte* payload, unsigned len)
{
  String t = String(topic);
  String msg = String((char*)payload).substring(0, len);

  /* ---  Slider volume (dB)  --- */
  if (t == volTopic) {
    float dB = msg.toFloat();
    volume   = constrain(lroundf(-2.0f * dB), 0, 100);
    if (!muted) audio.setVolume(volume);
    sendAll();
    Serial.printf("[MQTT] Volume slider : %.1f dB -> %d/100\n", dB, volume);
    return;
  }

  /* ---  URL libre  --- */
  if (t == urlTopic && len >= 8) {
    customURL    = msg;
    customActive = true;
    startCurrentStream();
    Serial.printf("[MQTT] URL custom : %s\n", customURL.c_str());
    return;
  }

  /* ---  Commandes UI / boutons  --- */
  if (t == cmdTopic && len >= 1) {
    char c = msg.charAt(0);

    /* Navigation presets */
    if (c == 'n') {                           // Chaine +
      presetIndex = (presetIndex + 1) % NB_PRESETS;
      customActive = false; startCurrentStream();
    }
    else if (c == 'v') {                      // Chaine –
      presetIndex = (presetIndex - 1 + NB_PRESETS) % NB_PRESETS;
      customActive = false; startCurrentStream();
    }
    else if (c >= '0' && c <= '6') {          // Choix direct (0-6)
      presetIndex = c - '0';
      customActive = false; startCurrentStream();
    }

    /* Volume par boutons */
    else if (c == '+') {
      if (volume < 100) { volume++; audio.setVolume(volume); }
      sendAll();
    }
    else if (c == '-') {
      if (volume > 0) { volume--; audio.setVolume(volume); }
      sendAll();
    }
    else if (c == 'm') {                      // MUTE
      if (!muted) { volumePrev = volume; audio.setVolume(100); volume = 0; }
      else         { volume = volumePrev;     audio.setVolume(volume);     }
      muted = !muted; sendAll();
    }

    /* STOP / REVENIR */
    else if (c == 'x') {                      // STOP
      audio.stopSong(); playing = false; sendAll();
    }
    else if (c == 'r') {                      // Revenir aux presets
      if (customActive) { customActive = false; startCurrentStream(); }
    }

    /* Bass / Treble / Tone reset */
    else if (c == 'g' && bassAmp < 15)  { bassAmp++; tone[3] = bassAmp; audio.setTone(tone); publishStatus(); }
    else if (c == 'f' && bassAmp > 0)   { bassAmp--; tone[3] = bassAmp; audio.setTone(tone); publishStatus(); }
    else if (c == 'j' && trebleAmp < 15){ trebleAmp++; tone[1] = trebleAmp; audio.setTone(tone); publishStatus(); }
    else if (c == 'h' && trebleAmp > 0) { trebleAmp--; tone[1] = trebleAmp; audio.setTone(tone); publishStatus(); }
    else if (c == 'd') {                      // reset graves/aigus
      trebleAmp = bassAmp = 5; tone[1] = trebleAmp; tone[3] = bassAmp;
      audio.setTone(tone); publishStatus();
    }

    /* Spatialisation */
    else if (c == 's') { toggleSpatial(); publishStatus(); }

    return;
  }
}

/* ----------  RECONNEXION MQTT ---------- */
void mqttReconnect()
{
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Radio")) {
      mqttClient.subscribe(cmdTopic);
      mqttClient.subscribe(volTopic);
      mqttClient.subscribe(urlTopic);
      sendAll();
    } else {
      delay(5000);
    }
  }
}

/* ----------  SETUP ---------- */
void setup()
{
  Serial.begin(115200);
  pinMode(VS1053_CS, OUTPUT);

  /* Wi-Fi via portail captif */
  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678"))
    ESP.restart();

  /* VS1053 */
  SPI.begin(); delay(500);
  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) ||
      !audio.isChipConnected()) {
    Serial.println("VS1053 non détecté !");
    while (true) delay(100);
  }
  audio.setVolume(volume);
  audio.setTone(tone);

  /* MQTT */
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  /* Première station */
  startCurrentStream();
}

/* ----------  LOOP ---------- */
void loop()
{
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();
  audio.loop();
}

/* ----------  Tableau récapitulatif des commandes ---------- *

Topic             Payload      Action
----------------  -----------  -----------------------------------------
radio/commande    n / v        Preset suivant / précédent
                  0 … 6        Accès direct au preset N
                  + / -        Volume +1 / –1 (-> publish slider)
                  m            Mute / Un-mute
                  r            Revenir (quitte l’URL custom)
                  x            STOP (arrêt du flux)
                  g / f        Bass + / Bass –
                  j / h        Treble + / Treble –
                  d            Grave/Aigu ↺ défaut (5)
                  s            Cycle spatialisation (OFF → LO → HI → LO+HI)
radio/volume_dB   « -7.5 »     Position du slider (dB, -50 dB → 0 dB)
radio/url         http://…     Lit l’URL comme 8ᵉ station spéciale
-------------------------------------------------------------------------- */





