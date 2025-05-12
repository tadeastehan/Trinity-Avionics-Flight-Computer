#include <Arduino.h>

// Define LED pins
const int ledPins[] = {7, 5, 6, 4}; // MTDO, MTDI, MTCK, MTMS
const int ledCount = sizeof(ledPins) / sizeof(ledPins[0]);
const int delayTime = 500; // milliseconds

void setup()
{
    // Initialize all LED pins as outputs
    for (int i = 0; i < ledCount; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
    }
}

void loop()
{
    // Light up each LED one by one
    for (int i = 0; i < ledCount; i++)
    {
        digitalWrite(ledPins[i], HIGH);
        delay(delayTime);
        digitalWrite(ledPins[i], LOW);
    }
}