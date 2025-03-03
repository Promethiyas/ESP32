#include <Arduino.h>

const int joystickXPin = 13;
const int joystickYPin = 14;
const int joystickButtonPin = 27;

void setup()
{
  Serial.begin(115200);
  pinMode(joystickButtonPin, INPUT_PULLUP);
}

void loop()
{
  int xValue = analogRead(joystickXPin);
  int yValue = analogRead(joystickYPin);
  int buttonValue = digitalRead(joystickButtonPin);

  Serial.print("Joystick X: ");
  Serial.print(xValue - 2000);
  Serial.print("  Joystick Y: ");
  Serial.print(yValue - 2000);
  Serial.print(" Button: ");
  Serial.println(buttonValue);

  delay(50);
}