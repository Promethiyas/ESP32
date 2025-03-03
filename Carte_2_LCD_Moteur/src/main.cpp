#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// --- Informations WiFi et MQTT ---
const char *ssid = "NIBBA4773";
const char *password = "12345678";

// Adresse du broker sur le réseau souhaité
const char *mqtt_server = "192.168.137.1"; // Broker sur la carte réseau "Connexion au réseau local* 3"
const int mqtt_port = 1883;
const char *mqtt_user = "user1";    // Optionnel
const char *mqtt_password = "1234"; // Optionnel

// Topics MQTT
const char *mqtt_topic_display = "esp32/display"; // Pour recevoir des messages d'affichage sur le LCD
const char *mqtt_topic_servo = "esp32/servo";     // Pour envoyer les valeurs du servo

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

// --- Définition des broches du servo moteur ---
static const int servoPin = 13;
Servo servo1;

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
  else if (String(topic) == mqtt_topic_servo)
  {
    int pos = message.toInt();
    servo1.write(pos);
    Serial.print("Position du servo : ");
    Serial.println(pos);
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
      client.subscribe(mqtt_topic_servo);
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

  // Initialisation du Servo
  servo1.attach(servoPin, 0, 4000);
}

void updateLCD()
{
  static String lastMessageStatic = "";
  static String lastMessageToScroll = "";
  static int lastScrollPos = -1;

  if (messageStatic != lastMessageStatic)
  {
    lcd.setCursor(0, 0);
    lcd.print(messageStatic + "      "); // On ajoute des espaces pour effacer d'anciens caractères
    lastMessageStatic = messageStatic;
  }

  if (isScrolling && millis() - lastScrollTime >= scrollSpeed)
  {
    lastScrollTime = millis();
    if (scrollPos != lastScrollPos || messageToScroll != lastMessageToScroll)
    {
      lcd.setCursor(0, 1);
      String displayText = messageToScroll.substring(scrollPos, scrollPos + 16);
      if (displayText.length() < 16)
      {
        displayText += "                " + messageToScroll.substring(0, 16 - displayText.length());
      }
      lcd.print(displayText);
      lastScrollPos = scrollPos;
      lastMessageToScroll = messageToScroll;
    }
    scrollPos++;
    if (scrollPos > messageToScroll.length())
    {
      scrollPos = 0;
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // --- Gestion de l'affichage sur le LCD ---
  updateLCD();
}
