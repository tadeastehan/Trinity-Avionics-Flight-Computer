#include <Arduino.h>
#include <ESP32Servo.h>
#include "board.h"
#include "env.h"

static unsigned long lastBuzzTime = 0;
bool buzzerState = LOW; // Initial state of the buzzer

float batteryVoltage = 0.0; // Variable to store battery voltage

void setupBoard()
{
    // Initialize RBF pin
    pinMode(RBF_PIN, INPUT_PULLUP);
    Serial.println("RBF pin initialized.");

    // Initialize Buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, buzzerState); // Turn off the buzzer initially
    Serial.println("Buzzer pin initialized.");

    // Initialize Battery pin
    pinMode(BATTERY_PIN, INPUT);
    Serial.println("Battery pin initialized.");

    // Initialize Servo
    ServoSetup();
}

void buzz()
{
    unsigned long currentTime = millis();
    // Buzz every second
    if (currentTime - lastBuzzTime >= BUZZING_INTERVAL)
    {
        lastBuzzTime = currentTime;
        buzzerState = !buzzerState;            // Toggle buzzer state
        digitalWrite(BUZZER_PIN, buzzerState); // Set the buzzer state
    }
}

void buzzOff()
{
    buzzerState = LOW; // Turn off the buzzer
    digitalWrite(BUZZER_PIN, buzzerState);
}

void updateBatteryVoltage()
{
    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++)
    {
        Vbatt += analogReadMilliVolts(BATTERY_PIN); // Read and accumulate ADC voltage
    }
    batteryVoltage = BATTERY_VOLTAGE_DIVIDER * (Vbatt / 16.0) / 1000.0; // Convert to voltage
}

float getBatteryVoltage()
{
    return batteryVoltage; // Return the current battery voltage
}

Servo myservo;

void ServoSetup()
{
    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myservo.setPeriodHertz(50);           // standard 50 hz servo
    myservo.attach(SERVO_PIN, 500, 2500); // attaches the servo on pin 18 to the servo object
    Serial.println("Servo attached");

    ServoOpen(); // Open the servo initially
    Serial.println("Servo opened.");
}

void ServoOpen()
{
    myservo.write(SERVO_OPENED_POSITION); // Open the servo
}

void ServoClose()
{
    myservo.write(SERVO_CLOSED_POSITION); // Close the servo
}