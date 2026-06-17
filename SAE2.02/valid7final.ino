#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP32_VS1053_Stream.h>
#include <SPI.h>

#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;
WiFiClient          espClient;
PubSubClient        mqttClient(espClient);

const char* mqttServer   = "broker.mqtt-dashboard.com";
const int   mqttPort     = 1883;

const char* cmdTopic     = "radio/commande";     
const char* volTopic     = "radio/volume_dB";    
const char* urlTopic     = "radio/url";         

const char* statusTopic  = "radio/statut";       
const char* bassTopic    = "radio/bass";
const char* trebleTopic  = "radio/treble";
const char* volStateTopic= "radio/volume_dB/state"; 

int   volume      = 85;
int   volumePrev  = 85;
bool  muted       = false;
bool  playing     = false;


#define NB_PRESETS 2
const char* presets[NB_PRESETS] = {
  "http://radios.rtbf.be/wr-c21-metal-128.mp3",
  "http://lyon1ere.ice.infomaniak.ch/lyon1ere-high.mp3"
};
int     presetIndex   = 0;
bool    customActive  = false;
String  customURL     = "";

uint8_t trebleAmp = 5, trebleFreq = 2;
uint8_t bassAmp   = 5, bassFreq   = 15;
uint8_t toneValues[4] = { trebleFreq, trebleAmp, bassFreq, bassAmp };
uint8_t spatial   = 0;  
bool suppressVolPublish = false;


void audio_info(const char *info)
{
  Serial.printf("[AUDIO] %s\n", info);

  if (strstr(info, "ICY 200 OK") || strstr(info, "Connected"))
    Serial.println("[AUDIO] >>> FLUX CONNECTÉ – la lecture va commencer <<<");
}


void audio_showstation(const char *station)
{
  Serial.printf("[AUDIO] Station : %s\n", station);
}

void audio_showstreaminfo(const char *infoStr)
{
  Serial.printf("[AUDIO] StreamInfo : %s\n", infoStr);
}

void publishLevels()
{
  if (!mqttClient.connected()) return;

  char buf[16];
  if (!suppressVolPublish) {
    dtostrf(-0.5f * volume, 0, 1, buf);
    mqttClient.publish(volStateTopic, buf, true);   
  }

  sprintf(buf, "%d", bassAmp);
  mqttClient.publish(bassTopic, buf, true);

  sprintf(buf, "%d", trebleAmp);
  mqttClient.publish(trebleTopic, buf, true);
}

void publishStatus()
{
  if (!mqttClient.connected()) return;

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

void startCurrentStream()
{
  audio.stopSong();
  delay(50);

  if (customActive) {
    Serial.printf("[PLAY] Station CUSTOM : %s\n", customURL.c_str());
    audio.connecttohost(customURL.c_str());
  } else {
    Serial.printf("[PLAY] Station #%d : %s\n", presetIndex, presets[presetIndex]);
    audio.connecttohost(presets[presetIndex]);
  }

  delay(1500);                    
  audio.setVolume(volume);
  audio.setTone(toneValues);

  uint8_t s = spatial; spatial = 255; spatial = s; toggleSpatial();

  playing = true;
  sendAll();
}

void mqttCallback(char* topic, byte* payload, unsigned len)
{
  String t   = String(topic);
  String msg = String((char*)payload).substring(0, len);

  if (t == volTopic) {
    float dB = msg.toFloat();
    volume   = constrain(lroundf((dB + 50.0f)*2.0f),0,100);
    if (!muted) audio.setVolume(volume);

    suppressVolPublish = true; 
    sendAll();
    suppressVolPublish = false;

    Serial.printf("[MQTT] Volume slider : %.1f dB -> %d/100\n", dB, volume);
    return;
  }

  if (t == urlTopic && len >= 8) {
    customURL    = msg;
    customActive = true;
    startCurrentStream();
    Serial.printf("[MQTT] URL custom : %s\n", customURL.c_str());
    return;
  }

  if (t == cmdTopic && len >= 1) {
    char c = msg.charAt(0);

    if (c == 'n')                    { presetIndex = (presetIndex + 1) % NB_PRESETS; customActive = false; startCurrentStream(); }
    else if (c == 'v')               { presetIndex = (presetIndex - 1 + NB_PRESETS) % NB_PRESETS; customActive = false; startCurrentStream(); }
    else if (c >= '0' && c <= '6')   { presetIndex = c - '0'; customActive = false; startCurrentStream(); }

    else if (c == '+') {
      if (volume < 100) { volume++; audio.setVolume(volume); }
      sendAll();
      Serial.printf("[CMD] Volume + -> %d/100\n", volume);
    }
    else if (c == '-') {
      if (volume > 0)   { volume--; audio.setVolume(volume); }
      sendAll();
      Serial.printf("[CMD] Volume - -> %d/100\n", volume);
    }
    else if (c == 'm') {
      if (!muted) { volumePrev = volume; audio.setVolume(100); volume = 0; Serial.println("[CMD] MUTE"); }
      else         { volume = volumePrev; audio.setVolume(volume); Serial.println("[CMD] UN-MUTE"); }
      muted = !muted; sendAll();
    }

    else if (c == 'x') { audio.stopSong(); playing = false; sendAll(); Serial.println("[CMD] STOP"); }
    else if (c == 'r') { if (customActive) { customActive = false; startCurrentStream(); Serial.println("[CMD] REVENIR aux presets"); } }

    else if (c == 'g' && bassAmp < 15)  { bassAmp++; toneValues[3] = bassAmp; audio.setTone(toneValues); sendAll(); Serial.printf("[CMD] Bass + -> %d\n", bassAmp); }
    else if (c == 'f' && bassAmp > 0)   { bassAmp--; toneValues[3] = bassAmp; audio.setTone(toneValues); sendAll(); Serial.printf("[CMD] Bass - -> %d\n", bassAmp); }
    else if (c == 'j' && trebleAmp < 15){ trebleAmp++; toneValues[1] = trebleAmp; audio.setTone(toneValues); sendAll(); Serial.printf("[CMD] Treble + -> %d\n", trebleAmp); }
    else if (c == 'h' && trebleAmp > 0) { trebleAmp--; toneValues[1] = trebleAmp; audio.setTone(toneValues); sendAll(); Serial.printf("[CMD] Treble - -> %d\n", trebleAmp); }
    else if (c == 'd') {
      trebleAmp = bassAmp = 5; toneValues[1] = trebleAmp; toneValues[3] = bassAmp;
      audio.setTone(toneValues); publishStatus(); Serial.println("[CMD] Graves/Aigus réinitialisés à 5");
    }

    else if (c == 's') { toggleSpatial(); publishStatus(); Serial.printf("[CMD] Spatial mode -> %d\n", spatial); }
  }
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentative de connexion au broker MQTT...");
    String clientId = "ESP32Radio-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connecté au broker MQTT");
      mqttClient.subscribe(cmdTopic);
      mqttClient.subscribe(volTopic);
      mqttClient.subscribe(urlTopic);
      Serial.print("Abonné au topic ");
      Serial.println(cmdTopic);
      Serial.println(volTopic);
      Serial.println(urlTopic);
      publishStatus();publishLevels(); 
    } else {
      Serial.print("Échec MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" → nouvelle tentative dans 5 s");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(VS1053_CS, OUTPUT);

  Serial.println("\n=== WebRadio ESP32 démarrage ===");

  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("[WIFI] échec, reboot…");
    ESP.restart();
  }
  Serial.printf("[WIFI] Connecté : %s (%s)\n",
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str());

  SPI.begin(); delay(500);
  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !audio.isChipConnected()) {
    Serial.println("VS1053 non détecté !");
    while (true) delay(100);
  }
  audio.setVolume(volume);
  audio.setTone(toneValues);
  Serial.println("[VS1053] OK");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  startCurrentStream();
}

void loop()
{
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();
  audio.loop();    
}