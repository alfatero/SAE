#include <WiFi.h>
#include <WiFiManager.h>
#include <ESP32_VS1053_Stream.h>
#include <SPI.h>

// Broches utilisées
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;

// Volume
int volume = 85;

// Stations
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

// Tonalité
uint8_t Treble_Amp = 5;
uint8_t Treble_Freq = 2;   // 2kHz
uint8_t Bass_Amp   = 5;
uint8_t Bass_Freq  = 15;   // 150Hz
uint8_t Tonalite[4] = { Treble_Freq, Treble_Amp, Bass_Freq, Bass_Amp };

// Spatialisation
#define SCI_MODE 0x00
#define SM_EARSPEAKER_LO (1 << 4)
#define SM_EARSPEAKER_HI (1 << 7)
uint8_t spatialMode = 0; // 0: off, 1: minimal, 2: normal, 3: extrême

// Fonctions SPI pour accéder aux registres SCI
uint16_t readSciRegister(uint8_t addr) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03); // Read
  SPI.transfer(addr);
  uint8_t hi = SPI.transfer(0xFF);
  uint8_t lo = SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return (hi << 8) | lo;
}

void writeSciRegister(uint8_t addr, uint16_t data) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02); // Write
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

  Serial.print("Spatialisation: ");
  switch (spatialMode) {
    case 0: Serial.println("OFF"); break;
    case 1: Serial.println("Minimal"); break;
    case 2: Serial.println("Normal"); break;
    case 3: Serial.println("Extrême"); break;
  }
}

void connexionChaine() {
  Serial.print("\nTentative de connexion à : ");
  Serial.println(urls[chaine]);

  audio.connecttohost(urls[chaine].c_str());
  delay(1500);  // attendre un peu que la connexion démarre

  if (!audio.isRunning()) {
    Serial.println("Échec de la connexion à la station !");
    Serial.println("Vérifie l'URL ou ta connexion réseau.");
  } else {
    Serial.println("Connexion réussie !");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nRadio WiFi avec tonalité et spatialisation");

  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("WiFi échec... Redémarrage");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.println(WiFi.localIP());

  SPI.begin();
  delay(1000);

  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !audio.isChipConnected()) {
    Serial.println("VS1053 non détecté !");
    while (1) delay(100);
  }

  audio.setVolume(volume);
  audio.setTone(Tonalite);

  Serial.println("Commandes :");
  Serial.println("  n = changer de chaîne");
  Serial.println("  + / - = volume");
  Serial.println("  g / f = basses + / -");
  Serial.println("  j / h = aigus + / -");
  Serial.println("  d = tonalité par défaut");
  Serial.println("  s = spatialisation");

  connexionChaine();
}

void loop() {
  audio.loop();

  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'n') {
      chaine = (chaine + 1) % NOMBRECHAINES;
      connexionChaine();
    }

    if (c == '+') {
      if (volume < 100) {
        volume++;
        audio.setVolume(volume);
        Serial.print("Volume: "); Serial.println(volume);
      }
    }

    if (c == '-') {
      if (volume > 0) {
        volume--;
        audio.setVolume(volume);
        Serial.print("Volume: "); Serial.println(volume);
      }
    }

    if (c == 'g' && Bass_Amp < 15) {
      Bass_Amp++;
      Serial.println("Basses +");
    }

    if (c == 'f' && Bass_Amp > 0) {
      Bass_Amp--;
      Serial.println("Basses -");
    }

    if (c == 'j' && Treble_Amp < 15) {
      Treble_Amp++;
      Serial.println("Aigus +");
    }

    if (c == 'h' && Treble_Amp > 0) {
      Treble_Amp--;
      Serial.println("Aigus -");
    }

    if (c == 'd') {
      Treble_Amp = 5;
      Bass_Amp = 5;
      Serial.println("Tonalité par défaut");
    }

    if (c == 's') {
      toggleSpatialisation();
    }

    Tonalite[0] = Treble_Freq;
    Tonalite[1] = Treble_Amp;
    Tonalite[2] = Bass_Freq;
    Tonalite[3] = Bass_Amp;
    audio.setTone(Tonalite);
  }
}
