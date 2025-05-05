#include <Arduino.h>
#include <FastLED.h>

// Define the LED pin
#define LED_PIN 4                       // ESP32C6 pin connected to WS2812E
#define NUM_LEDS 1                      // Number of LED units (just one in this case)
#define LED_TYPE WS2811Controller800Khz // LED strip type (WS2812E is compatible with WS2812B settings)
#define COLOR_ORDER GRB                 // RGB order for these LEDs

// Define the LED array
CRGB leds[NUM_LEDS];

// Variables for demo patterns
unsigned long lastChangeTime = 0;
int patternState = 0;
int brightnessLevel = 255; // Full brightness to start with
int colorIndex = 0;

// Predefined colors array
CRGB colorArray[] = {
    CRGB::Red,
    CRGB::Green,
    CRGB::Blue,
    // CRGB::Yellow,
    // CRGB::Purple,
    // CRGB::Cyan,
    // CRGB::White,
    // CRGB::Orange
};

// Number of colors in the array
#define NUM_COLORS (sizeof(colorArray) / sizeof(colorArray[0]))

void setupRGBLight()
{
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightnessLevel);

  // Set initial color to red
  leds[0] = CRGB::Red;
  FastLED.show();

  Serial.println("RGB LED initialized on pin 4");
}

void showColor(CRGB color, int brightness)
{
  FastLED.setBrightness(brightness);
  leds[0] = color;
  FastLED.show();
}

// Gradually fade between colors
void fadeToColor(CRGB targetColor, int steps, int delayMs)
{
  CRGB startColor = leds[0];

  for (int i = 0; i <= steps; i++)
  {
    leds[0].r = startColor.r + ((targetColor.r - startColor.r) * i / steps);
    leds[0].g = startColor.g + ((targetColor.g - startColor.g) * i / steps);
    leds[0].b = startColor.b + ((targetColor.b - startColor.b) * i / steps);
    FastLED.show();
    delay(delayMs);
  }
}

// Cycle through different brightness levels
void cycleBrightness()
{
  static int brightness = 255;
  static int direction = -1;

  brightness += direction * 5;

  if (brightness <= 10)
  {
    brightness = 10;
    direction = 1;
  }
  else if (brightness >= 255)
  {
    brightness = 255;
    direction = -1;
  }

  FastLED.setBrightness(brightness);
  FastLED.show();
  delay(20);
}

// Demo patterns
void updateRGBLight()
{
  unsigned long currentTime = millis();

  // Change pattern every 5 seconds
  if (currentTime - lastChangeTime > 5000)
  {
    lastChangeTime = currentTime;
    patternState = (patternState + 1) % 4; // Cycle through 4 pattern states
    Serial.print("Changing to pattern: ");
    Serial.println(patternState);

    // Reset brightness at the start of each pattern
    FastLED.setBrightness(255);
  }

  switch (patternState)
  {
  case 0: // Solid color changing
    if (currentTime - lastChangeTime < 100)
    { // Only change at the beginning of the state
      colorIndex = (colorIndex + 1) % NUM_COLORS;
      showColor(colorArray[colorIndex], 255);
      Serial.print("Color: ");
      Serial.println(colorIndex);
    }
    break;

  case 1: // Fading brightness of current color
    cycleBrightness();
    break;

  case 2: // Smooth color transitions
    fadeToColor(colorArray[(colorIndex + 1) % NUM_COLORS], 100, 20);
    colorIndex = (colorIndex + 1) % NUM_COLORS;
    break;

  case 3:                                            // Rainbow effect
    leds[0] = CHSV((millis() / 20) % 255, 255, 255); // Hue cycles through rainbow
    FastLED.show();
    delay(10);
    break;
  }
}

// Simplified function that just shows solid colors at 10% brightness
// and changes color every 5 seconds
void updateSolidRGBLight()
{
  static unsigned long lastColorChangeTime = 0;
  static int currentColorIndex = 0;
  unsigned long currentTime = millis();

  // Change color every 5 seconds
  if (currentTime - lastColorChangeTime > 1000)
  {
    lastColorChangeTime = currentTime;

    // Move to the next color
    currentColorIndex = (currentColorIndex + 1) % NUM_COLORS;

    // Set the color with 10% brightness (25.5 rounded to 26)
    FastLED.setBrightness(255); // 10% of 255
    leds[0] = colorArray[currentColorIndex];
    FastLED.show();

    Serial.print("Changed to color: ");
    Serial.println(currentColorIndex);
  }
}

// To use this file in your project, include the following in your main.cpp:
// 1. Add these lines at the top of your file:
//    extern void setupRGBLight();
//    extern void updateRGBLight();
//    extern void updateSolidRGBLight();  // If you want the simpler version
//
// 2. Call setupRGBLight() in your setup() function
// 3. Call updateRGBLight() or updateSolidRGBLight() in your loop() function

// If you want to use this file as a standalone program, uncomment the following:

void setup()
{
  Serial.begin(115200);
  delay(2000); // Power-up safety delay

  Serial.println("WS2812E RGB LED Demo");
  setupRGBLight();
}

void loop()
{
  // Choose one of these functions to use:
  // updateRGBLight();     // Complex patterns with multiple effects
  updateSolidRGBLight(); // Simple solid colors at 10% brightness
}