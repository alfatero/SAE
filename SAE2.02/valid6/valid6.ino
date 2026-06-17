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

const char* mqttServer     = "broker.mqtt-dashboard.com";
const int   mqttPort       = 1883;
const char* commandeTopic  = "radio/commande";
const char* statutTopic    = "radio/statut";

int volume = 85;

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

uint8_t Treble_Amp = 5;
uint8_t Treble_Freq = 2;
uint8_t Bass_Amp   = 5;
uint8_t Bass_Freq  = 15;
uint8_t Tonalite[4] = { Treble_Freq, Treble_Amp, Bass_Freq, Bass_Amp };

#define SCI_MODE 0x00
#define SM_EARSPEAKER_LO (1 << 4)
#define SM_EARSPEAKER_HI (1 << 7)
uint8_t spatialMode = 0;

uint16_t readSciRegister(uint8_t addr) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03);
  SPI.transfer(addr);
  uint8_t hi = SPI.transfer(0xFF);
  uint8_t lo = SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return (hi << 8) | lo;
}

void writeSciRegister(uint8_t addr, uint16_t data) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02);
  SPI.transfer(addr);
  SPI.transfer(data >> 8);
  SPI.transfer(data & 0xFF);
  digitalWrite(VS1053_CS, HIGH);
}

void toggleSpatialisation() {
  uint16_t mode = readSciRegister(SCI_MODE);
  mode &= ~(SM_EARSPEAKER_LO | SM_EARSPEAKER_HI);
  spatialMode = (spatialMode + 1) % 4;
  switch (spatialMode) {
    case 1: mode |= SM_EARSPEAKER_LO; break;
    case 2: mode |= SM_EARSPEAKER_HI; break;
    case 3: mode |= (SM_EARSPEAKER_LO | SM_EARSPEAKER_HI); break;
  }
  writeSciRegister(SCI_MODE, mode);
  Serial.print("Spatialisation -> ");
  switch (spatialMode) {
    case 0: Serial.println("OFF"); break;
    case 1: Serial.println("Minimal"); break;
    case 2: Serial.println("Normal"); break;
    case 3: Serial.println("Extrême"); break;
  }
}

void publishStatus() {
  char buffer[128];
  snprintf(buffer, sizeof(buffer),
    "station=%d,volume=%d,treble_amp=%d,bass_amp=%d,spatial=%d",
    chaine, volume, Treble_Amp, Bass_Amp, spatialMode);
  mqttClient.publish(statutTopic, buffer);
}

void connexionChaine() {
  Serial.print("Connexion à la station ");
  Serial.print(chaine);
  Serial.print(" : ");
  Serial.println(urls[chaine]);

  audio.stopSong();
  delay(50);
  audio.connecttohost(urls[chaine].c_str());
  delay(1500);

  if (!audio.isRunning()) {
    Serial.println("❌ Échec de connexion au flux !");
  } else {
    Serial.println("✅ Connexion au flux réussie");
  }

  audio.setVolume(volume);
  audio.setTone(Tonalite);

  // Réappliquer spatialisation sans imprimer deux fois
  uint8_t savedMode = spatialMode;
  spatialMode = 255; // forcer toggleSpatialisation à imprimer
  toggleSpatialisation();
  spatialMode = savedMode;
  toggleSpatialisation();

  publishStatus();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (String(topic) != commandeTopic) return;
  if (length < 1) return;
  char c = (char)payload[0];

  if (c == 'n') {
    Serial.println("Commande MQTT: station suivante");
    chaine = (chaine + 1) % NOMBRECHAINES;
    connexionChaine();
  }
  else if (c == 'v') {
    Serial.println("Commande MQTT: station précédente");
    chaine = (chaine - 1 + NOMBRECHAINES) % NOMBRECHAINES;
    connexionChaine();
  }
  else if (c == '+') {
    if (volume < 100) {
      volume++;
      audio.setVolume(volume);
      Serial.print("Volume augmenté -> ");
      Serial.println(volume);
      publishStatus();
    }
  }
  else if (c == '-') {
    if (volume > 0) {
      volume--;
      audio.setVolume(volume);
      Serial.print("Volume diminué -> ");
      Serial.println(volume);
      publishStatus();
    }
  }
  else if (c == 'g' && Bass_Amp < 15) {
    Bass_Amp++;
    Serial.print("Basses augmentées -> ");
    Serial.println(Bass_Amp);
  }
  else if (c == 'f' && Bass_Amp > 0) {
    Bass_Amp--;
    Serial.print("Basses diminuées -> ");
    Serial.println(Bass_Amp);
  }
  else if (c == 'j' && Treble_Amp < 15) {
    Treble_Amp++;
    Serial.print("Aigus augmentés -> ");
    Serial.println(Treble_Amp);
  }
  else if (c == 'h' && Treble_Amp > 0) {
    Treble_Amp--;
    Serial.print("Aigus diminués -> ");
    Serial.println(Treble_Amp);
  }
  else if (c == 'd') {
    Treble_Amp = 5;
    Bass_Amp   = 5;
    Serial.println("Tonalité réinitialisée aux valeurs par défaut");
  }
  else if (c == 's') {
    Serial.println("Commande MQTT: basculer spatialisation");
    toggleSpatialisation();
  }

  Tonalite[0] = Treble_Freq;
  Tonalite[1] = Treble_Amp;
  Tonalite[2] = Bass_Freq;
  Tonalite[3] = Bass_Amp;
  audio.setTone(Tonalite);
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentative de connexion au broker MQTT...");
    String clientId = "ESP32Radio-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connecté au broker MQTT");
      mqttClient.subscribe(commandeTopic);
      Serial.print("Abonné au topic ");
      Serial.println(commandeTopic);
      publishStatus();
    } else {
      Serial.print("Échec MQTT, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" → nouvelle tentative dans 5 s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(VS1053_CS, OUTPUT);

  Serial.println("\nDémarrage Radio WiFi + MQTT");

  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("❌ Échec WiFi → redémarrage");
    ESP.restart();
  }
  Serial.print("Connecté au WiFi, IP : ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  delay(1000);

  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ)
      || !audio.isChipConnected()) {
    Serial.println("❌ VS1053 non détecté");
    while (1) delay(100);
  }
  Serial.println("✅ VS1053 détecté");

  audio.setVolume(volume);
  audio.setTone(Tonalite);

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  connexionChaine();
}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  audio.loop();
}
