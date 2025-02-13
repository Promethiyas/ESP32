#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// WiFi
const char *ssid = "A54 de Valentin"; // Nom de ton réseau Wi-Fi
const char *password = "Platypus";  // Mot de passe Wi-Fi

// MQTT Broker
const char *mqtt_broker = "192.168.134.210";
const char *topic = "esp32";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

MFRC522 mfrc522(5, 9);   // Crée une instance MFRC522

String mqtt_message;

void callback(char *topic, byte *payload, unsigned int length) {
    mqtt_message = "";
    Serial.print("Message reçu dans le topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
        mqtt_message += (char)payload[i];
    }
    Serial.println();
    Serial.println("-----------------------");
}

// --- Reconnexion MQTT ---
void reconnect()
{
  // Boucle jusqu'à connexion au broker
  while (!client.connected())
  {
    Serial.print("Tentative de connexion MQTT...");
    if (client.connect("ESP32Client"/*, mqtt_user, mqtt_password*/))
    {
      Serial.println("Connecté");
      client.subscribe(topic);
    }
    else
    {
      Serial.print("Échec, code erreur = ");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connexion au WiFi...");
  }
  Serial.println("Connecté au réseau WiFi");

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("Le client %s se connecte au broker MQTT\n", client_id.c_str());
    if (client.connect(client_id.c_str()/*, mqtt_username, mqtt_password*/)) {
      Serial.println("Connecté au broker MQTT");
    } else {
      Serial.print("Échec, état: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  client.subscribe(topic);

  SPI.begin();
  mfrc522.PCD_Init();   // Init la carte MFRC522
  Serial.println(F("Écrire des données sur une carte MIFARE PICC "));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Préparer la clé - toutes les clés sont définies à FFFFFFFFFFFFh à la livraison depuis l'usine.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("UID de la carte:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print(F(" Type de PICC: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;

  // Utiliser la valeur du topic MQTT comme nom de famille
  String familyName = mqtt_message;
  familyName.toCharArray((char *)buffer, 30);

  block = 1;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Échec de l'authentification: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println(F("Authentification réussie"));

  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Échec de l'écriture: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println(F("Écriture réussie"));

  mfrc522.PICC_HaltA(); // Mettre en pause la carte PICC
  mfrc522.PCD_StopCrypto1();  // Arrêter le chiffrement sur PCD
}
