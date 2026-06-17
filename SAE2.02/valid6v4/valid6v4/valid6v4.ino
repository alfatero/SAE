#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP32_VS1053_Stream.h>
#include <SPI.h>

#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// broker et topics
const char* mqttServer     = "broker.mqtt-dashboard.com";
const int   mqttPort       = 1883;
const char* commandeTopic  = "radio/commande";    // commandes + slider
const char* statutTopic    = "radio/statut";      // état global
const char* volTopic       = "radio/volume_dB";   // état du volume (dB)
const char* bassTopic      = "radio/bass";        // état des basses
const char* trebleTopic    = "radio/treble";      // état des aigus

// état du lecteur
int volume      = 85;   // 0..100 (0=-50dB, 100=0dB)
int volumeprec  = 0;
bool muted      = false;

#define NOMBRECHAINES 7
String urls[NOMBRECHAINES] = {
  "http://stream03.ustream.ca:8000/cism128.mp3",
  "http://chisou-02.cdn.eurozet.pl:8112/;",
  "http://streamer01.sti.usherbrooke.ca:8000/cfak.mp3",
  "http://radios.rtbf.be/wr-c21-metal-128.mp3",
  "http://ecoutez.chyz.ca:8000/mp3",
  "http://www.skyrock.fm/stream.php/tunein16_128mp3.mp3",
  "http://lyon1ere.ice.infomaniak.ch/lyon1ere-high.mp3"
};
int chaine = 0;

// tonalité & spatialisation
uint8_t Treble_Amp = 5, Treble_Freq = 2;
uint8_t Bass_Amp   = 5, Bass_Freq   = 15;
uint8_t Tonalite[4] = { Treble_Freq, Treble_Amp, Bass_Freq, Bass_Amp };
uint8_t spatialMode = 0;

// accès aux registres SCI du VS1053
uint16_t readSciRegister(uint8_t addr) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03); SPI.transfer(addr);
  uint16_t hi = SPI.transfer(0xFF), lo = SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return (hi<<8)|lo;
}
void writeSciRegister(uint8_t addr, uint16_t data) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02); SPI.transfer(addr);
  SPI.transfer(data>>8); SPI.transfer(data&0xFF);
  digitalWrite(VS1053_CS, HIGH);
}

// toggle spatialisation
void toggleSpatialisation() {
  const uint16_t SM_LO=(1<<4), SM_HI=(1<<7);
  uint16_t m = readSciRegister(0);
  m &= ~(SM_LO|SM_HI);
  spatialMode = (spatialMode+1)%4;
  if (spatialMode&1) m |= SM_LO;
  if (spatialMode&2) m |= SM_HI;
  writeSciRegister(0, m);
}

// publie l’état global (station, volume pas, tonalité, spatial)
void publishStatus() {
  char buf[128];
  snprintf(buf,sizeof(buf),
    "station=%d,volume=%d,treble=%d,bass=%d,spatial=%d",
    chaine, volume, Treble_Amp, Bass_Amp, spatialMode);
  mqttClient.publish(statutTopic, buf, true);
}

// publie les indicateurs (volume en dB, bass, treble)
void publishLevels() {
  // volume en dB
  float dB = -volume * 0.5f;
  char buf[16];
  dtostrf(dB, 0, 1, buf);
  mqttClient.publish(volTopic, buf, true);
  // bass
  sprintf(buf, "%d", Bass_Amp);
  mqttClient.publish(bassTopic, buf, true);
  // treble
  sprintf(buf, "%d", Treble_Amp);
  mqttClient.publish(trebleTopic, buf, true);
}

// change ou reconnecte à la chaîne
void connexionChaine() {
  audio.stopSong(); delay(50);
  audio.connecttohost(urls[chaine].c_str());
  delay(1500);
  audio.setVolume(volume);
  audio.setTone(Tonalite);
  // réapplique brièvement la spatialisation
  uint8_t s=spatialMode; spatialMode=255; spatialMode=s;
  toggleSpatialisation();
  publishStatus();
  publishLevels();
}

// callback unifié pour slider+boutons
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String msg = String((char*)payload).substring(0,length);

  // 1) si c'est un slider (float dB)
  if (t == volTopic && length>0) {
    float dB = msg.toFloat();             // "-12.5" dB
    int vol = round(-2.0f * dB);          // pas 0..100
    vol = constrain(vol, 0, 100);
    volume = vol;
    audio.setVolume(volume);
    publishStatus();
    publishLevels();
    Serial.printf("Slider→%.1f dB → %d/100\n", dB, volume);
    return;
  }

  // 2) sinon, commandes sur commandeTopic
  if (t == commandeTopic && length>=1) {
    char c = msg.charAt(0);
    if (c=='n') {
      chaine = (chaine+1)%NOMBRECHAINES;
      connexionChaine();
    }
    else if (c=='v') {
      chaine = (chaine-1+NOMBRECHAINES)%NOMBRECHAINES;
      connexionChaine();
    }
    else if (c=='+') {
      if (volume<100) {
        volume++;
        audio.setVolume(volume);
        publishStatus();
        publishLevels();
      }
    }
    else if (c=='-') {
      if (volume>0) {
        volume--;
        audio.setVolume(volume);
        publishStatus();
        publishLevels();
      }
    }
    else if (c=='m') {
      if (!muted) {
        volumeprec=volume;
        audio.setVolume(100);
        volume=0;
      } else {
        volume=volumeprec;
        audio.setVolume(volume);
      }
      muted=!muted;
      publishStatus();
      publishLevels();
    }
    else if (c=='g' && Bass_Amp<15) {
      Bass_Amp++;
      Tonalite[3]=Bass_Amp;
      audio.setTone(Tonalite);
      publishStatus();
    }
    else if (c=='f' && Bass_Amp>0) {
      Bass_Amp--;
      Tonalite[3]=Bass_Amp;
      audio.setTone(Tonalite);
      publishStatus();
    }
    else if (c=='j' && Treble_Amp<15) {
      Treble_Amp++;
      Tonalite[1]=Treble_Amp;
      audio.setTone(Tonalite);
      publishStatus();
    }
    else if (c=='h' && Treble_Amp>0) {
      Treble_Amp--;
      Tonalite[1]=Treble_Amp;
      audio.setTone(Tonalite);
      publishStatus();
    }
    else if (c=='d') {
      Treble_Amp=5;
      Bass_Amp=5;
      Tonalite[1]=Treble_Amp;
      Tonalite[3]=Bass_Amp;
      audio.setTone(Tonalite);
      publishStatus();
    }
    else if (c=='s') {
      toggleSpatialisation();
      publishStatus();
    }
  }
}

// gère la reconnexion MQTT
void mqttReconnect() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Radio")) {
      mqttClient.subscribe(commandeTopic);
      mqttClient.subscribe(volTopic);
      publishStatus();
      publishLevels();
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(VS1053_CS, OUTPUT);

  // WiFiManager pour le WiFi
  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32","12345678")) ESP.restart();

  // initialisation VS1053
  SPI.begin(); delay(500);
  if (!audio.startDecoder(VS1053_CS,VS1053_DCS,VS1053_DREQ)
      || !audio.isChipConnected()) {
    while (1) delay(100);
  }
  audio.setVolume(volume);
  audio.setTone(Tonalite);

  // MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  // première connexion
  connexionChaine();
}

void loop() {
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();
  audio.loop();
}

