#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>

// Informations de connexion WiFi
const char *ssid = "NIBBA4773";
const char *password = "12345678";

// Création d'un serveur HTTP sur le port 80
WebServer server(80);

// Configuration de l'écran LCD
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Message à afficher²
String messageStatic = "Coucou!";
String messageToScroll = "";
int scrollSpeed = 300; // Default scroll speed in milliseconds

// Fonction de connexion WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi ...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nConnecté avec IP : " + WiFi.localIP().toString());
}

// Fonction pour faire défiler le texte sur l'écran LCD
void scrollText(int row, String message, int delayTime, int lcdColumns)
{
  for (int i = 0; i < lcdColumns; i++)
  {
    message = " " + message;
  }
  message += " ";
  for (int pos = 0; pos < message.length(); pos++)
  {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// Fonction pour faire défiler le texte sur l'écran LCD en sens inverse
void scrollTextReverse(int row, String message, int delayTime, int lcdColumns)
{
  for (int i = 0; i < lcdColumns; i++)
  {
    message += " ";
  }
  for (int pos = message.length() - lcdColumns; pos >= 0; pos--)
  {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// Gestion de la requête HTTP pour mettre à jour le texte et la vitesse de défilement
void handleMessage()
{
  if (server.hasArg("text"))
  {
    messageToScroll = server.arg("text");
    Serial.println("Nouveau message reçu: " + messageToScroll);
    server.send(200, "text/plain", "Message reçu: " + messageToScroll);
  }
  else
  {
    server.send(400, "text/plain", "Erreur : aucun message reçu.");
  }

  if (server.hasArg("speed"))
  {
    scrollSpeed = server.arg("speed").toInt();
    Serial.println("Nouvelle vitesse de défilement: " + String(scrollSpeed) + " ms");
  }
}

void setup()
{
  Serial.begin(115200);
  initWiFi();

  // Initialisation du LCD
  lcd.init();
  lcd.backlight();

  // Configuration du serveur Web
  server.on("/", HTTP_GET, []()
            { server.send(200, "text/html",
                          "<h1>Envoyez un message à l'ESP32</h1>"
                          "<form action='/message' method='GET'>"
                          "Message: <input type='text' name='text'><br>"
                          "Vitesse de défilement (ms): <input type='number' name='speed' min='0'><br>"
                          "<input type='submit' value='Envoyer'>"
                          "</form>"); });

  server.on("/message", HTTP_GET, handleMessage);
  server.begin();
}

void loop()
{
  server.handleClient();

  // Afficher un message statique
  lcd.setCursor(0, 0);
  lcd.print(messageStatic);

  // Afficher le message reçu en scrollant
  if (messageToScroll.length() > 0)
  {
    scrollText(1, messageToScroll, scrollSpeed, lcdColumns);
  }
}
