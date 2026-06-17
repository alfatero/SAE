/*  ESP32 WebRadio + VS1053  – v3.2 (06/2025)
 *  ----------------------------------------------------------------
 *  - 7 présélections + 1 URL libre (topic radio/url)
 *  - Commandes MQTT : voir tableau plus bas
 *  - Volume modifiable par slider (dB) OU par boutons +- / mute
 *  - Fonctions STOP et REVENIR
 *  - Publie systématiquement le statut pour garder l’IHM à jour
 *  - Corrige la boucle infinie sur le topic volume
 *  - Ajoute des traces série détaillées (station, tonalité, spatialisation, réseau)
 *  - NEW v3.2 : callbacks audio_* pour savoir exactement quand le flux est OK
 */

#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ESP32_VS1053_Stream.h>
#include <SPI.h>

/************************  Configuration matérielle  *************************/
#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   15

ESP32_VS1053_Stream audio;
WiFiClient          espClient;
PubSubClient        mqttClient(espClient);

/************************  Topics MQTT  **************************************/
const char* mqttServer   = "broker.mqtt-dashboard.com";
const int   mqttPort     = 1883;

// Entrées (IHM → Radio)
const char* cmdTopic     = "radio/commande";     // Commandes diverses
const char* volTopic     = "radio/volume_dB";    // Position du slider
const char* urlTopic     = "radio/url";          // URL custom

// Sorties (Radio → IHM)
const char* statusTopic  = "radio/statut";       // Statut global
const char* bassTopic    = "radio/bass";
const char* trebleTopic  = "radio/treble";
const char* volStateTopic= "radio/volume_dB/state"; // *** évite la boucle ***

/************************  État lecteur  *************************************/
int   volume      = 85;   // 0-100  ( +1 = –0,5 dB )
int   volumePrev  = 85;
bool  muted       = false;
bool  playing     = false;

/************************  Stations présélectionnées  ************************/
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
int     presetIndex   = 0;
bool    customActive  = false;
String  customURL     = "";

/************************  Tonalité & spatialisation  ************************/
uint8_t trebleAmp = 5, trebleFreq = 2;
uint8_t bassAmp   = 5, bassFreq   = 15;
uint8_t toneValues[4] = { trebleFreq, trebleAmp, bassFreq, bassAmp };
uint8_t spatial   = 0;    // 0 = OFF, 1 = LO, 2 = HI, 3 = LO+HI

/************************  Variables internes  *******************************/
// Permet de bloquer la repub du volume quand l'info vient déjà du slider
bool suppressVolPublish = false;

/************************  ===  Callbacks AUDIO  ============================*/
/*  La librairie appelle ces fonctions SANS qu’on ait besoin de les enregistrer
 *  (elles sont vues comme des symboles faibles). On peut donc simplement les
 *  définir pour récupérer toutes les infos voulues.
 */
void audio_info(const char *info)
{
  Serial.printf("[AUDIO] %s\n", info);

  // Petit repère visuel lorsque le handshake ICY est terminé
  if (strstr(info, "ICY 200 OK") || strstr(info, "Connected"))
    Serial.println("[AUDIO] >>> FLUX CONNECTÉ – la lecture va commencer <<<");
}

// Infos station (nom)
void audio_showstation(const char *station)
{
  Serial.printf("[AUDIO] Station : %s\n", station);
}

// Infos sur le flux (bitrate, codec…)
void audio_showstreaminfo(const char *infoStr)
{
  Serial.printf("[AUDIO] StreamInfo : %s\n", infoStr);
}

// Titre de la piste courante (quand dispo)
void audio_showstreamtitle(const char *title)
{
  Serial.printf("[AUDIO] StreamTitle : %s\n", title);
}

/************************  OUTILS  *******************************************/
void publishLevels()
{
  if (!mqttClient.connected()) return;

  char buf[16];
  // Publication du volume (en dB) – sauf si appel déjà issu du slider
  if (!suppressVolPublish) {
    dtostrf(-0.5f * volume, 0, 1, buf);
    mqttClient.publish(volStateTopic, buf, true);   // topic distinct
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

/************************  VS1053 : spatialisation  **************************/
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

/************************  LECTURE STATION  **********************************/
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

  delay(1500);                    // petit temps pour le buffer
  audio.setVolume(volume);
  audio.setTone(toneValues);

  uint8_t s = spatial; spatial = 255; spatial = s; toggleSpatial(); // refresh spatial

  playing = true;
  sendAll();
}

/************************  CALLBACK MQTT  ************************************/
void mqttCallback(char* topic, byte* payload, unsigned len)
{
  String t   = String(topic);
  String msg = String((char*)payload).substring(0, len);

  /*****  Gestion volume (slider)  *****/
  if (t == volTopic) {
    float dB = msg.toFloat();
    volume   = constrain(lroundf(-2.0f * dB), 0, 100);
    if (!muted) audio.setVolume(volume);

    suppressVolPublish = true; // évite la boucle ↺
    sendAll();
    suppressVolPublish = false;

    Serial.printf("[MQTT] Volume slider : %.1f dB -> %d/100\n", dB, volume);
    return;
  }

  /*****  URL custom  *****/
  if (t == urlTopic && len >= 8) {
    customURL    = msg;
    customActive = true;
    startCurrentStream();
    Serial.printf("[MQTT] URL custom : %s\n", customURL.c_str());
    return;
  }

  /*****  Commandes diverses  *****/
  if (t == cmdTopic && len >= 1) {
    char c = msg.charAt(0);

    // ----------  Stations  ----------
    if (c == 'n')                    { presetIndex = (presetIndex + 1) % NB_PRESETS; customActive = false; startCurrentStream(); }
    else if (c == 'v')               { presetIndex = (presetIndex - 1 + NB_PRESETS) % NB_PRESETS; customActive = false; startCurrentStream(); }
    else if (c >= '0' && c <= '6')   { presetIndex = c - '0'; customActive = false; startCurrentStream(); }

    // ----------  Volume boutons  ----------
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

    // ----------  Stop / Revenir ----------
    else if (c == 'x') { audio.stopSong(); playing = false; sendAll(); Serial.println("[CMD] STOP"); }
    else if (c == 'r') { if (customActive) { customActive = false; startCurrentStream(); Serial.println("[CMD] REVENIR aux presets"); } }

    // ----------  Bass / Treble ----------
    else if (c == 'g' && bassAmp < 15)  { bassAmp++; toneValues[3] = bassAmp; audio.setTone(toneValues); publishStatus(); Serial.printf("[CMD] Bass + -> %d\n", bassAmp); }
    else if (c == 'f' && bassAmp > 0)   { bassAmp--; toneValues[3] = bassAmp; audio.setTone(toneValues); publishStatus(); Serial.printf("[CMD] Bass - -> %d\n", bassAmp); }
    else if (c == 'j' && trebleAmp < 15){ trebleAmp++; toneValues[1] = trebleAmp; audio.setTone(toneValues); publishStatus(); Serial.printf("[CMD] Treble + -> %d\n", trebleAmp); }
    else if (c == 'h' && trebleAmp > 0) { trebleAmp--; toneValues[1] = trebleAmp; audio.setTone(toneValues); publishStatus(); Serial.printf("[CMD] Treble - -> %d\n", trebleAmp); }
    else if (c == 'd') {
      trebleAmp = bassAmp = 5; toneValues[1] = trebleAmp; toneValues[3] = bassAmp;
      audio.setTone(toneValues); publishStatus(); Serial.println("[CMD] Graves/Aigus réinitialisés à 5");
    }

    // ----------  Spatialisation ----------
    else if (c == 's') { toggleSpatial(); publishStatus(); Serial.printf("[CMD] Spatial mode -> %d\n", spatial); }
  }
}

/************************  RECONNEXION MQTT  *********************************/
void mqttReconnect()
{
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Connexion au broker… ");
    if (mqttClient.connect("ESP32Radio")) {
      Serial.println("OK");
      mqttClient.subscribe(cmdTopic);
      mqttClient.subscribe(volTopic);
      mqttClient.subscribe(urlTopic);
      sendAll();
    } else {
      Serial.printf("KO (rc=%d). Nouvel essai dans 5 s…\n", mqttClient.state());
      delay(5000);
    }
  }
}

/************************  SETUP  ********************************************/
void setup()
{
  Serial.begin(115200);
  pinMode(VS1053_CS, OUTPUT);

  Serial.println("\n=== WebRadio ESP32 démarrage ===");

  // -----  Wi-Fi  -----
  WiFiManager wm;
  if (!wm.autoConnect("WebRadioESP32", "12345678")) {
    Serial.println("[WIFI] échec, reboot…");
    ESP.restart();
  }
  Serial.printf("[WIFI] Connecté : %s (%s)\n",
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str());

  // -----  VS1053  -----
  SPI.begin(); delay(500);
  if (!audio.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !audio.isChipConnected()) {
    Serial.println("VS1053 non détecté !");
    while (true) delay(100);
  }
  audio.setVolume(volume);
  audio.setTone(toneValues);
  Serial.println("[VS1053] OK");

  // -----  MQTT  -----
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);

  startCurrentStream();
}

/************************  LOOP  *********************************************/
void loop()
{
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();
  audio.loop();     // indispensable pour le buffering du flux
}

/************************  Tableau récapitulatif des commandes  **************
Topic             Payload      Action
----------------  -----------  -----------------------------------------
radio/commande    n / v        Preset suivant / précédent
                  0 … 6        Accès direct au preset N
                  + / -        Volume +1 / –1 (→ publish slider)
                  m            Mute / Un-mute
                  r            Revenir (quitte l’URL custom)
                  x            STOP (arrêt du flux)
                  g / f        Bass + / Bass –
                  j / h        Treble + / Treble –
                  d            Grave/Aigu ↺ défaut (5)
                  s            Cycle spatialisation (OFF → LO → HI → LO+HI)
radio/volume_dB   « -7.5 »     Position du slider (dB, -50 → 0 dB)
radio/url         http://…     Lit l’URL comme 8ᵉ station spéciale
******************************************************************************/
