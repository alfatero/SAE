const int pinBouton = 14;   // Bouton sur GPIO13
const int pinLED    = 12;   // LED sur GPIO15
// 13 data capteur

bool etatLED = false;       // Mémorise l'état de la LED
bool dernierEtatBouton = HIGH; // Stocke l'état précédent du bouton (HIGH = pas appuyé)

void setup() {
  pinMode(pinBouton, INPUT_PULLUP); // Bouton avec résistance interne
  pinMode(pinLED, OUTPUT);          // LED en sortie
  digitalWrite(pinLED, LOW);        // LED éteinte au départ
}

void loop() {
  bool lectureBouton = digitalRead(pinBouton);

  // Détection du front descendant (bouton pressé une seule fois)
  if (dernierEtatBouton == HIGH && lectureBouton == LOW) {
    etatLED = !etatLED;                 // Inverse l'état de la LED
    digitalWrite(pinLED, etatLED ? HIGH : LOW); // Applique le nouvel état
    delay(50); // Anti-rebond simple
  }

  dernierEtatBouton = lectureBouton; // Mémorise l'état du bouton
}








