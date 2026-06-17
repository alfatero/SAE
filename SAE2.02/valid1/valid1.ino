/*******************************************************************
  Web radio simple à base d'ESP32 et VS1053
  Basé sur un sketch de Vince Gellár (github.com/vincegellar)
  Bibliotheque VS1053 de baldram (https://github.com/baldram/ESP_VS1053_Library)
  Plus d'infos:  https://electroniqueamateur.blogspot.com/2021/03/esp32-et-vs1053-ecouter-la-radio-sur.html
*********************************************************************/

#include <VS1053.h>
#include <WiFi.h>

// broches utilisées
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

// nom et mot de passe de votre réseau:
const char *ssid = "testleandro";
const char *password = "alfatero96";

#define BUFFSIZE 64  //32, 64 ou 128
uint8_t mp3buff[BUFFSIZE];

int volume = 85;  // volume sonore 0 à 100

#define NOMBRECHAINES 7 // nombre de chaînes prédéfinies
int chaine = 0; //station actuellement sélectionnée

//caractéristiques de la station actuellement sélectionnée
char host[40];
char path[40];
int httpPort;

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);
WiFiClient client;

// connexion à une chaine
void connexionChaine () {

  switch (chaine) {
    case 0:
      strcpy(host, "stream03.ustream.ca");
      strcpy(path, "/cism128.mp3");
      httpPort = 8000;
      break;
      
    case 1:
      strcpy(host, "chisou-02.cdn.eurozet.pl");
      strcpy(path, "/;");
      httpPort = 8112;
      break;
      
    case 2:
      strcpy(host, "streamer01.sti.usherbrooke.ca");
      strcpy(path, "/cfak.mp3");
      httpPort = 8000;
      break;

    case 3:
      strcpy(host, "radios.rtbf.be");
      strcpy(path, "/wr-c21-metal-128.mp3");
      httpPort = 80;
      break;

    case 4:
      strcpy(host, "ecoutez.chyz.ca");
      strcpy(path, "/mp3");
      httpPort = 8000;
      break;

    case 5:
      strcpy(host, "ice4.somafm.com");
      strcpy(path, "/seventies-128-mp3");
      httpPort = 80;
      break;

    case 6:
      strcpy(host, "lyon1ere.ice.infomaniak.ch");
      strcpy(path, "/lyon1ere-high.mp3");
      httpPort = 80;
      break;
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

  Serial.println("\n\nRadio WiFi");
  Serial.println("");

  Serial.println("Controles: ");
  Serial.println("\t n: synthoniser une autre chaine");
  Serial.println("\t + / -: controle du volume");

  Serial.print("Connexion au reseau ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connecte");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());

  SPI.begin();

  player.begin();
  player.switchToMp3Mode();
  player.setVolume(volume);

  connexionChaine();

}

void loop() {

  if (Serial.available()) {
    char c = Serial.read();

    // n: prochaine chaine
    if (c == 'n') {
      Serial.println("On change de chaine");
      client.stop();
      if (chaine < (NOMBRECHAINES - 1)) {
        chaine++;
      }
      else { // retour au début de la liste
        chaine = 0;
      }
      connexionChaine();
    }

    // +: augmenter le volume
    if (c == '+') {
      if (volume < 100) {
        Serial.println("Plus fort");
        volume++;
        player.setVolume(volume);
      }
    }

    // -: diminuer le volume
    if (c == '-') {
      if (volume > 0) {
        Serial.println("Moins fort");
        volume--;
        player.setVolume(volume);
      }
    }
  }

  if (client.available() > 0) {
    uint8_t bytesread = client.read(mp3buff, BUFFSIZE);
    if (bytesread) {
      player.playChunk(mp3buff, bytesread);
    }
  }
}
