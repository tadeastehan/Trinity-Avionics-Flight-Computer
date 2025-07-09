#include <Arduino.h>
#include <ESP32Servo.h>
#include <FastLED.h>
#include "board.h"
#include "env.h"

// Define the LED pin
#define LED_PIN 4                       // ESP32C6 pin connected to WS2812E
#define NUM_LEDS 1                      // Number of LED units (just one in this case)
#define LED_TYPE WS2811Controller800Khz // LED strip type (WS2812E is compatible with WS2812B settings)
#define COLOR_ORDER GRB                 // RGB order for these LEDs

// Define the LED array
CRGB leds[NUM_LEDS];

static unsigned long lastRGBShowTime = 0; // Stores the last time showRGBColor was called
bool rgbLightEnabled = false;             // Flag to indicate if RGB light is enabled

// Checks if the RGB LED should be cleared after 5 seconds
void checkRGBState()
{
    if (rgbLightEnabled && (millis() - lastRGBShowTime >= 5000))
    {
        clearRGBLight(); // Clear the RGB light if it has been more than 5 seconds since the last update
    }
}

void showRGBColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    FastLED.setBrightness(brightness); // Set the brightness level

    // Set the color of the first LED
    leds[0] = CRGB(red, green, blue);
    FastLED.show();
    lastRGBShowTime = millis(); // Record the time of this call
    rgbLightEnabled = true;     // Set the flag to indicate RGB light is enabled
}

void clearRGBLight()
{
    rgbLightEnabled = false; // Reset the flag to indicate RGB light is cleared
    leds[0] = CRGB::Black;   // Clear the LED array
    FastLED.show();          // Update the LEDs to show the cleared state
}

void setupRGBLight()
{
    // Initialize FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

static unsigned long buzzStartTime = 0;
static unsigned long buzzDuration = 0;
static bool buzzActive = false;

float batteryVoltage = 0.0; // Variable to store battery voltage

void setupBoard()
{
    // Initialize RBF pin
    pinMode(RBF_PIN, INPUT_PULLUP);
    Serial.println("RBF pin initialized.");

    // Initialize Buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer initially
    Serial.println("Buzzer pin initialized.");

    // Initialize Battery pin
    pinMode(BATTERY_PIN, INPUT);
    Serial.println("Battery pin initialized.");

    // Initialize RGB Light
    setupRGBLight();
    Serial.println("RGB Light initialized.");

    // Initialize Servo
    ServoSetup();
    Serial.println("Servo initialized.");
}

// Start buzzing for a given duration (non-blocking)
void buzz(int duration)
{
    buzzStartTime = millis();
    buzzDuration = duration;
    buzzActive = true;
    digitalWrite(BUZZER_PIN, HIGH);
}

// Enable or disable interval buzzing (interval is always BUZZING_INTERVAL)
static bool intervalBuzzEnabled = false;

// Check if the buzzer should be turned off or handle interval buzzing
void checkBuzzState()
{
    static unsigned long lastBuzzToggle = 0;
    static bool buzzOn = false;

    // Handle interval buzzing
    if (intervalBuzzEnabled)
    {
        if (!buzzActive && !buzzOn && (millis() - lastBuzzToggle >= BUZZING_INTERVAL))
        {
            lastBuzzToggle = millis();
            buzz(BUZZING_INTERVAL / 2); // Buzz ON for half the interval
            buzzOn = true;
        }
        if (buzzOn && !buzzActive)
        {
            // Buzz finished, turn off and wait for next interval
            buzzOn = false;
        }
    }

    // Handle single buzz
    if (buzzActive && (millis() - buzzStartTime >= buzzDuration))
    {
        digitalWrite(BUZZER_PIN, LOW);
        buzzActive = false;
    }
}

void intervalBuzzEnable()
{
    intervalBuzzEnabled = true;
}

void intervalBuzzDisable()
{
    intervalBuzzEnabled = false;
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