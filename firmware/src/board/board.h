#pragma once

#include <Arduino.h>

// Initializes board pins and peripherals
void setupBoard();

// Toggles the buzzer state every second
// Start buzzing for a given duration (non-blocking)
void buzz(int duration);
// Check if the buzzer should be turned off
void checkBuzzState();
// Enable interval buzzing (non-blocking, uses BUZZING_INTERVAL)
void intervalBuzzEnable();
// Disable interval buzzing
void intervalBuzzDisable();

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

void setupRGBLight(); // Call in setup() to initialize RGB light

// Updates the RGB light state and records the time of the last call
void showRGBColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness = 255);

// Clear the RGB light state
void clearRGBLight(); // Call to clear the RGB light state

// Checks if the RGB LED should be cleared after 5 seconds
void checkRGBState();