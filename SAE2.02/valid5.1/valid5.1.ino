#include <WiFi.h>
#include <WiFiManager.h>
#include <ESP32_VS1053_Stream.h>

// Broches utilisées pour le VS1053
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;  // constructeur sans paramètres

int volume = 85;

// Liste des URL
#define NOMBRECHAINES 7
String urls[NOMBRECHAINES] = {
  "http://stream03.ustream.ca:8000/cism128.mp3",
  "http://chisou-02.cdn.eurozet.pl:8112/;",
  "http://streamer01.sti.usherbrooke.ca:8000/cfak.mp3",
  "http://radios.rtbf.be/wr-c21-metal-128.mp3",
  "http://ecoutez.chyz.ca:8000/mp3",
  "http://ice4.somafm.com/seventies-128-mp3",
  "http://lyon1ere.ice.infomaniak.ch/lyon1ere-high.mp3"
};

int chaine = 0;

void connexionChaine() {
  Serial.print("Connexion à : ");
  Serial.println(urls[chaine]);
  audio.connecttohost(urls[chaine].c_str());
}

// Optionnel : pour afficher des infos
void audio_info(const char* info) {
  Serial.print("info: "); Serial.println(info);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nRadio WiFi avec URL complète\n");

  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("Échec de connexion WiFi... Redémarrage.");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  delay(1000);  // temps pour démarrer correctement le VS1053

  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !audio.isChipConnected()) {
    Serial.println("Erreur VS1053 !");
    while (1) delay(100);
  }

  audio.setVolume(volume);
  // audio.setMetadataCallback(audio_info);  // si tu veux utiliser les callbacks

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
  }
}



