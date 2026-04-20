#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Definition for LED control
#define MAX_LED_OUTPUTS 3
#define MAX_LEDS_PER_OUTPUT 300

#define LED_PIN     38      // Legacy primary data pin alias (output 1)
#define NUM_LEDS    1       // Legacy primary output LED count alias
#define BRIGHTNESS  5       // Brightness (0-255)

// Color change speed
#define DELAY_MS    1000    // Time between color changes in milliseconds

// Function declarations
void setupLEDs();
void updateLEDs();

// Multi-output pin and count configuration
extern int LED_GPIO_PINS[MAX_LED_OUTPUTS];
extern int LED_COUNTS[MAX_LED_OUTPUTS];

bool setActiveLedOutput(uint8_t ledId);
uint8_t getActiveLedOutput();

// New functions for manual color control
void setColorByIndex(int index);     // Sets color by predefined index
void setColorRGB(int r, int g, int b); // Sets color with RGB values
void setColorHSV(int h, int s, int v); // Sets color with HSV values
bool setColorByIndexForLed(uint8_t ledId, int index);
bool setColorRGBForLed(uint8_t ledId, int r, int g, int b);
bool setColorHSVForLed(uint8_t ledId, int h, int s, int v);

// New function for brightness control
void setBrightness(int brightness);  // Sets LED brightness (0-255)
bool setBrightnessForLed(uint8_t ledId, int brightness); // Brightness targeting specific LED output

#endif // LED_CONTROL_H