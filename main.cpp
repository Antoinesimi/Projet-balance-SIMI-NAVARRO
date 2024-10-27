// importation des bibliothèques
#include <HX711.h>      // bibliothèque du capteur
#include "Arduino.h"    //bibliothèque arduino
#include "PubSubClient.h"   //bibliothèque MQTT
#include "WiFi.h"   //bibliothèque Wifi
#include "Wire.h"   //bibliothèque I2C

// Déclarations

// Pins de branchement du capteur sur la carte 
const int DIGITAL_OUTPUT = 21;     // Pin signal de données
const int HORLOGE = 22;     // Pin de signal d'horloge du circuit

//variable de résultat
float masse = 0;

// Paramètres MQTT Broker 

const int mqtt_port = 1883;   
const char *mqtt_broker = "147.94.219.220"; // Identifiant du broker (Adresse IP)
const char *topic = "balance"; // Nom du topic sur lequel les données seront envoyés 

WiFiClient espClient; 
PubSubClient client(espClient); 

// Paramètres EDUROAM 
#define EAP_IDENTITY "antoine.simi@etu.univ-amu.fr"
#define EAP_PASSWORD "Tonito13!" 
#define EAP_USERNAME "antoine.simi@etu.univ-amu.fr" 
const char* ssid = "eduroam"; // eduroam SSID

// Fonction réception du message MQTT 
void callback(char *topic, byte *payload, unsigned int length) { 
  Serial.print("Le message a été envoyé sur le topic : "); 
  Serial.println(topic); 
  Serial.print("Message:"); 
  for (int i = 0; i < length; i++) { 
    Serial.print((char) payload[i]); 
  } 
  Serial.println(); 
  Serial.println("-----------------------"); 
}

HX711 scale;

// Déclarations liées à la moyenne glissante
const int nb_valeurs = 10;    // nombre de valeurs du tableau
float tab_valeurs[nb_valeurs];    //tableau de valeurs
int i = 0;    //variable d'incrémentation du tableau
float somme = 0;    // variable somme des valeurs du tableau
float moyenne = 0;   // variable résultat de la moyenne

void setup() {
  
  Serial.begin(115200);
  scale.begin(DIGITAL_OUTPUT, HORLOGE);     // Démarrage de la communication entre la carte et le capteur
  scale.set_scale(882.f);      // Valeur obtenue en calibrant le capteur avec des masses connues, elle permet de convertir la valeur renvoyée par le capteur en masse réelle 
  scale.tare();     // Remise à 0 de l'échelle (tare)

  // mise à 0 des valeurs du tableau
  for (int k = 0; k < nb_valeurs; k++) {
    tab_valeurs[k] = 0;
  }
// Connexion au réseau EDUROAM 

  WiFi.disconnect(true);
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD); 
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println("");
  Serial.println(F("L'ESP32 est connecté au WiFi !"));
  
// Connexion au broker MQTT  
  
  client.setServer(mqtt_broker, mqtt_port); 
  client.setCallback(callback); 

  while (!client.connected()) { 
    String client_id = "esp32-client-"; 
    client_id += String(WiFi.macAddress()); 
    Serial.printf("La chaîne de mesure %s se connecte au broker MQTT", client_id.c_str()); 
 
    if (client.connect(client_id.c_str())) { 
      Serial.println("La chaîne de mesure est connectée au broker."); 
    } else { 
      Serial.print("La chaîne de mesure n'a pas réussi à se connecter ... "); 
      Serial.print(client.state()); 
      delay(2000); 
    } 
  } 
}

void loop() { 
    
  if (scale.is_ready()) {     // Condition prêt à mesurer ?
    
    masse = (scale.get_units()*0.1*9.81);      // calcul de la tension de surface

    somme -= tab_valeurs[i];    // enlève la première valeur du tableau au calcul de la somme des valeurs
    tab_valeurs[i] = masse;   // insère la nouvelle valeur au tableau
    somme += tab_valeurs[i];    // calcul de la somme des valeurs du tableau avec la nouvelle valeur
    i += 1;   // incrémentation

    // Remise à 0 de la variable d'incrémentation i si elle dépassse le nombre de valeurs du tableau
    if (i >= nb_valeurs) {
      i = 0;
    }
    
    moyenne = somme / nb_valeurs;    // résultat de la moyenne des valeurs du tableau

  client.publish(topic, String(moyenne).c_str()); // Publication de la masse sur le topic 
  client.subscribe(topic); // S'abonne au topic pour recevoir des messages
  client.loop(); // Gère les messages MQTT 
  delay(200); // Pause entre chaque envoi
}
  
   else {
    
  client.publish(topic, String("Erreur").c_str()); // Publication de la masse sur le topic 
  client.subscribe(topic); // S'abonne au topic pour recevoir des messages
  client.loop(); // Gère les messages MQTT 
  delay(200); // Pause entre chaque envoi
  }
}

