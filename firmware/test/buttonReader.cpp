#include <Arduino.h>

#define BUTTON_PIN 7

void setup()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.begin(115200);
}

void loop()
{
    int buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW)
    {
        Serial.println("Button pressed");
    }
    else
    {
        Serial.println("Button released");
    }
    delay(200);
}