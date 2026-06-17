#include <VS1053.h>
#include <WiFi.h>
#include <WiFiManager.h> //configuration WiFi via portail captif

// broches utilisées
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

// Buffers et audio
#define BUFFSIZE 64
uint8_t mp3buff[BUFFSIZE];
int volume = 85;
#define NOMBRECHAINES 7
int chaine = 0;

char host[40];
char path[40];
int httpPort;

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);
WiFiClient client;

// Tonalité
uint8_t Treble_Amp = 5;
uint8_t Treble_Freq = 2;
uint8_t Bass_Amp = 5;
uint8_t Bass_Freq = 15;
uint8_t Tonalite[4] = {Treble_Freq, Treble_Amp, Bass_Freq, Bass_Amp};

// Registre et bits de spatialisation
#define SCI_MODE 0x00
#define SM_EARSPEAKER_LO (1 << 4)
#define SM_EARSPEAKER_HI (1 << 7)
uint8_t spatialMode = 0;  // 0: off, 1: minimal, 2: normal, 3: extrême

// Fonctions SPI pour lire/écrire dans un registre SCI
uint16_t readSciRegister(uint8_t addr) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03); // Read opcode
  SPI.transfer(addr);
  uint8_t hi = SPI.transfer(0xFF);
  uint8_t lo = SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return (hi << 8) | lo;
}

void writeSciRegister(uint8_t addr, uint16_t data) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02); // Write opcode
  SPI.transfer(addr);
  SPI.transfer(data >> 8);
  SPI.transfer(data & 0xFF);
  digitalWrite(VS1053_CS, HIGH);
}

// Fonction de changement de mode de spatialisation
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

  Serial.print("Mode spatialisation: ");
  switch (spatialMode) {
    case 0: Serial.println("OFF"); break;
    case 1: Serial.println("Minimal"); break;
    case 2: Serial.println("Normal"); break;
    case 3: Serial.println("Extrême"); break;
  }
}

void connexionChaine() {
  switch (chaine) {
    case 0: strcpy(host, "stream03.ustream.ca"); strcpy(path, "/cism128.mp3"); httpPort = 8000; break;
    case 1: strcpy(host, "chisou-02.cdn.eurozet.pl"); strcpy(path, "/;"); httpPort = 8112; break;
    case 2: strcpy(host, "streamer01.sti.usherbrooke.ca"); strcpy(path, "/cfak.mp3"); httpPort = 8000; break;
    case 3: strcpy(host, "radios.rtbf.be"); strcpy(path, "/wr-c21-metal-128.mp3"); httpPort = 80; break;
    case 4: strcpy(host, "ecoutez.chyz.ca"); strcpy(path, "/mp3"); httpPort = 8000; break;
    case 5: strcpy(host, "ice4.somafm.com"); strcpy(path, "/seventies-128-mp3"); httpPort = 80; break;
    case 6: strcpy(host, "lyon1ere.ice.infomaniak.ch"); strcpy(path, "/lyon1ere-high.mp3"); httpPort = 80; break;
  }

  Serial.print("Connection a ");
  Serial.println(host);

  if (!client.connect(host, httpPort)) {
    Serial.println("Echec de la connexion");
    return;
  }

  Serial.print("Demande du stream: ");
  Serial.println(path);

  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nRadio WiFi\n");

  Serial.println("Controles:");
  Serial.println("  n: prochaine chaine");
  Serial.println("  + / -: volume");
  Serial.println("  g/f: basses + / -");
  Serial.println("  j/h: aigus + / -");
  Serial.println("  d: tonalité par défaut");
  Serial.println("  s: spatialisation");
  
  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("Échec de connexion au WiFi... Redémarrage.");
    ESP.restart();
  }

  Serial.println("Connecté au WiFi !");
  Serial.println("Adresse IP : ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  delay(500); // laisser le temps au VS1053

  player.begin();
  player.switchToMp3Mode();
  player.setVolume(volume);
  player.setTone(Tonalite);

  connexionChaine();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 'n') {
      Serial.println("On change de chaine");
      client.stop();
      chaine = (chaine + 1) % NOMBRECHAINES;
      connexionChaine();
    }

    if (c == '+') {
      if (volume < 100) {
        Serial.println("Plus fort");
        volume++;
        player.setVolume(volume);
      }
    }

    if (c == '-') {
      if (volume > 0) {
        Serial.println("Moins fort");
        volume--;
        player.setVolume(volume);
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

    // Mise à jour de la tonalité
    Tonalite[0] = Treble_Freq;
    Tonalite[1] = Treble_Amp;
    Tonalite[2] = Bass_Freq;
    Tonalite[3] = Bass_Amp;
    player.setTone(Tonalite);
  }

  if (client.available() > 0) {
    uint8_t bytesread = client.read(mp3buff, BUFFSIZE);
    if (bytesread) {
      player.playChunk(mp3buff, bytesread);
    }
  }
}


