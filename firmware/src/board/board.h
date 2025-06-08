#pragma once

#include <Arduino.h>

// Initializes board pins and peripherals
void setupBoard();

// Toggles the buzzer state every second
void buzz();
// Turn off the buzzer
void buzzOff();

// Updates the battery voltage reading
void updateBatteryVoltage();
// Returns the current battery voltage
float getBatteryVoltage();

// Servo
void ServoSetup(); // Call in setup() to initialize servo
// Open servo
void ServoOpen(); // Call to open the servo
// Close servo
void ServoClose(); // Call to close the servo
