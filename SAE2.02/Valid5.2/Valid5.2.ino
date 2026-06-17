#include <WiFi.h>
#include <WiFiManager.h>
#include <ESP32_VS1053_Stream.h>

#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

#define SCI_MODE 0x00
#define SM_EARSPEAKER_LO (1 << 4)
#define SM_EARSPEAKER_HI (1 << 7)

ESP32_VS1053_Stream audio;

int volume = 85;

uint8_t Treble_Amp = 5;
uint8_t Treble_Freq = 2;
uint8_t Bass_Amp = 5;
uint8_t Bass_Freq = 15;
uint8_t Tonalite[4] = {Treble_Freq, Treble_Amp, Bass_Freq, Bass_Amp};

uint8_t spatialMode = 0;

#define NOMBRECHAINES 7
String urls[NOMBRECHAINES] = {
  "http://stream03.ustream.ca:8000/cism128.mp3",
  "http://chisou-02.cdn.eurozet.pl:8112/;",
  "http://streamer01.sti.usherbrooke.ca:8000/cfak.mp3",
  "http://radios.rtbf.be/wr-c21-metal-128.mp3",
  "http://ecoutez.chyz.ca:8000/mp3",
  "http://icecast.skyrock.net/s/natio_mp3_128k",
  "http://lyon1ere.ice.infomaniak.ch/lyon1ere-high.mp3"
};

int chaine = 0;

void connexionChaine() {
  Serial.print("Connexion à : ");
  Serial.println(urls[chaine]);

  bool success = audio.connecttohost(urls[chaine].c_str());

  if (success) {
    Serial.println("✅ Connexion au flux réussie !");
  } else {
    Serial.println("❌ Échec de connexion au flux radio !");
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println("\n\nRadio WiFi avec URL complète + tonalité + spatialisation\n");

  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("Échec de connexion WiFi... Redémarrage.");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  delay(1000);

  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !audio.isChipConnected()) {
    Serial.println("Erreur VS1053 !");
    while (1) delay(100);
  }

  audio.setVolume(volume);
  audio.setTone(Tonalite);

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
        Serial.print("Volume: ");
        Serial.println(volume);
      }
    }

    if (c == '-') {
      if (volume > 0) {
        volume--;
        audio.setVolume(volume);
        Serial.print("Volume: ");
        Serial.println(volume);
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
  

    Tonalite[0] = Treble_Freq;
    Tonalite[1] = Treble_Amp;
    Tonalite[2] = Bass_Freq;
    Tonalite[3] = Bass_Amp;
    audio.setTone(Tonalite);
  }
}
