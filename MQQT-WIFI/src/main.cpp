#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- Informations WiFi et MQTT ---
const char *ssid = "NIBBA4773";
const char *password = "12345678";

// Adresse du broker sur le réseau souhaité
const char *mqtt_server = "192.168.137.1"; // Broker sur la carte réseau "Connexion au réseau local* 3"
const int mqtt_port = 1883;
const char *mqtt_user = "user1";    // Optionnel
const char *mqtt_password = "1234"; // Optionnel

// Topics MQTT
const char *mqtt_topic_display = "esp32/display";   // Pour recevoir des messages d'affichage sur le LCD
const char *mqtt_topic_joystick = "esp32/joystick"; // Pour envoyer les valeurs du joystick

WiFiClient espClient;
PubSubClient client(espClient);

// --- Configuration de l'écran LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
String messageStatic = "Coucou!";
String messageToScroll = "";
int scrollSpeed = 300; // en ms
unsigned long lastScrollTime = 0;
int scrollPos = 0;
bool isScrolling = false;

// --- Définition des broches du joystick ---
const int JOYSTICK_X_PIN = 32;
const int JOYSTICK_Y_PIN = 33;
const int JOYSTICK_CLICK_PIN = 27;
unsigned long lastJoystickSend = 0;
unsigned long joystickSendInterval = 500; // Envoi toutes les 500ms

// --- Connexion au WiFi ---
void setup_wifi()
{
  Serial.println();
  Serial.print("Connexion au WiFi ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connecté, IP : ");
  Serial.println(WiFi.localIP());
}

// --- Callback MQTT ---
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message reçu sur le topic ");
  Serial.print(topic);
  Serial.print(" : ");
  String message = "";
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Si le message provient du topic d'affichage, on met à jour le texte à scroller sur le LCD.
  if (String(topic) == mqtt_topic_display)
  {
    messageToScroll = " " + message + " ";
    scrollPos = 0;
    isScrolling = true;
  }
}

// --- Reconnexion MQTT ---
void reconnect()
{
  // Boucle jusqu'à connexion au broker
  while (!client.connected())
  {
    Serial.print("Tentative de connexion MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password))
    {
      Serial.println("Connecté");
      // S'abonner au topic d'affichage
      client.subscribe(mqtt_topic_display);
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

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Initialisation du LCD
  lcd.init();
  lcd.backlight();

  // Initialisation du joystick
  // Le bouton (click) utilise une résistance interne pull-up
  pinMode(JOYSTICK_CLICK_PIN, INPUT_PULLUP);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // --- Gestion de l'affichage sur le LCD ---
  lcd.setCursor(0, 0);
  lcd.print(messageStatic + "      "); // On ajoute des espaces pour effacer d'anciens caractères

  if (isScrolling && millis() - lastScrollTime >= scrollSpeed)
  {
    lastScrollTime = millis();
    lcd.setCursor(0, 1);
    lcd.print(messageToScroll.substring(scrollPos, scrollPos + 16));
    scrollPos++;
    if (scrollPos > messageToScroll.length() - 16)
    {
      scrollPos = 0;
    }
  }

  // --- Lecture du joystick et envoi des données via MQTT ---
  if (millis() - lastJoystickSend >= joystickSendInterval)
  {
    lastJoystickSend = millis();
    int xValue = analogRead(JOYSTICK_X_PIN);
    int yValue = analogRead(JOYSTICK_Y_PIN);
    int clickValue = digitalRead(JOYSTICK_CLICK_PIN);

    // Formatage en JSON
    String joystickMsg = "{\"x\": " + String(xValue) +
                         ", \"y\": " + String(yValue) +
                         ", \"click\": " + String(clickValue) + "}";

    Serial.print("Données joystick : ");
    Serial.println(joystickMsg);

    client.publish(mqtt_topic_joystick, joystickMsg.c_str());
  }
}
