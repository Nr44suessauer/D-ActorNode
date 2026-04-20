#include "led_control.h"

// Multi-output LED configuration (can be overridden by pin_config)
int LED_GPIO_PINS[MAX_LED_OUTPUTS] = {38, 39, 40};
int LED_COUNTS[MAX_LED_OUTPUTS] = {1, 1, 1};

// Runtime strips for each output (pin and count can be changed at runtime)
static Adafruit_NeoPixel* strips[MAX_LED_OUTPUTS] = {nullptr, nullptr, nullptr};
static uint32_t currentColors[MAX_LED_OUTPUTS] = {0, 0, 0};
static uint8_t brightnessPerOutput[MAX_LED_OUTPUTS] = {BRIGHTNESS, BRIGHTNESS, BRIGHTNESS};

static uint8_t activeLedOutput = 1;

// Define various colors
uint32_t colorList[] = {
  Adafruit_NeoPixel::Color(255, 0, 0),
  Adafruit_NeoPixel::Color(0, 255, 0),
  Adafruit_NeoPixel::Color(0, 0, 255),
  Adafruit_NeoPixel::Color(255, 255, 0),
  Adafruit_NeoPixel::Color(128, 0, 128),
  Adafruit_NeoPixel::Color(255, 165, 0),
  Adafruit_NeoPixel::Color(255, 255, 255)
};

int currentColorIndex = 0;
unsigned long previousMillis = 0;

static bool isValidLedId(uint8_t ledId) {
  return ledId >= 1 && ledId <= MAX_LED_OUTPUTS;
}

static int sanitizeLedCount(int count) {
  if (count < 1) return 1;
  if (count > MAX_LEDS_PER_OUTPUT) return MAX_LEDS_PER_OUTPUT;
  return count;
}

static void initOutput(uint8_t ledId) {
  if (!isValidLedId(ledId)) return;

  const int idx = ledId - 1;
  LED_COUNTS[idx] = sanitizeLedCount(LED_COUNTS[idx]);

  if (strips[idx] != nullptr) {
    delete strips[idx];
    strips[idx] = nullptr;
  }

  strips[idx] = new Adafruit_NeoPixel(LED_COUNTS[idx], LED_GPIO_PINS[idx], NEO_GRB + NEO_KHZ800);
  strips[idx]->begin();
  strips[idx]->setBrightness(brightnessPerOutput[idx]);
}

static void applyColorToOutput(uint8_t ledId, uint32_t color) {
  if (!isValidLedId(ledId)) return;
  const int idx = ledId - 1;
  if (strips[idx] == nullptr) return;

  currentColors[idx] = color;
  const int count = strips[idx]->numPixels();
  for (int i = 0; i < count; i++) {
    strips[idx]->setPixelColor(i, color);
  }
  strips[idx]->show();
}

// Existing functions remain unchanged
void setupLEDs() {
  // Initialize all LED outputs with current pin/count configuration
  for (int i = 0; i < MAX_LED_OUTPUTS; i++) {
    initOutput(i + 1);
    applyColorToOutput(i + 1, colorList[currentColorIndex]);
  }
  
  Serial.printf("LED outputs initialized: O1@IO%d x%d, O2@IO%d x%d, O3@IO%d x%d\n",
                LED_GPIO_PINS[0], LED_COUNTS[0],
                LED_GPIO_PINS[1], LED_COUNTS[1],
                LED_GPIO_PINS[2], LED_COUNTS[2]);
}

void updateLEDs() {
  unsigned long currentMillis = millis();
  
  // Check if it's time for a color change
  if (currentMillis - previousMillis >= DELAY_MS) {
    previousMillis = currentMillis;
    
    // Select next color
    currentColorIndex = (currentColorIndex + 1) % (sizeof(colorList) / sizeof(colorList[0]));
    
    // Set color on active output
    applyColorToOutput(activeLedOutput, colorList[currentColorIndex]);
    
    // Output current color
    Serial.print("Color changed to: ");
    switch (currentColorIndex) {
      case 0: Serial.println("Red"); break;
      case 1: Serial.println("Green"); break;
      case 2: Serial.println("Blue"); break;
      case 3: Serial.println("Yellow"); break;
      case 4: Serial.println("Purple"); break;
      case 5: Serial.println("Orange"); break;
      case 6: Serial.println("White"); break;
    }
  }
}

// New functions for color control

// Set color by index (0=Red, 1=Green, etc.)
void setColorByIndex(int index) {
  setColorByIndexForLed(activeLedOutput, index);
}

bool setColorByIndexForLed(uint8_t ledId, int index) {
  if (!isValidLedId(ledId)) return false;

  // Ensure index is valid
  int maxIndex = sizeof(colorList) / sizeof(colorList[0]) - 1;
  index = constrain(index, 0, maxIndex);
  
  // Set color
  currentColorIndex = index;
  applyColorToOutput(ledId, colorList[currentColorIndex]);
  
  // Debug output
  Serial.print("LED Output ");
  Serial.print(ledId);
  Serial.print(" color manually set to: ");
  switch (currentColorIndex) {
    case 0: Serial.println("Red"); break;
    case 1: Serial.println("Green"); break;
    case 2: Serial.println("Blue"); break;
    case 3: Serial.println("Yellow"); break;
    case 4: Serial.println("Purple"); break;
    case 5: Serial.println("Orange"); break;
    case 6: Serial.println("White"); break;
    default: Serial.println("Unknown");
  }
  return true;
}

// Set color with RGB values (0-255 for each component)
void setColorRGB(int r, int g, int b) {
  setColorRGBForLed(activeLedOutput, r, g, b);
}

bool setColorRGBForLed(uint8_t ledId, int r, int g, int b) {
  if (!isValidLedId(ledId)) return false;

  // Set RGB values
  applyColorToOutput(ledId, Adafruit_NeoPixel::Color(r, g, b));
  
  // Debug output
  Serial.print("LED Output ");
  Serial.print(ledId);
  Serial.print(" color manually set to RGB: ");
  Serial.print(r); Serial.print(", ");
  Serial.print(g); Serial.print(", ");
  Serial.println(b);
  return true;
}

// Set color with HSV values (hue 0-255, saturation 0-255, brightness 0-255)
void setColorHSV(int h, int s, int v) {
  setColorHSVForLed(activeLedOutput, h, s, v);
}

bool setColorHSVForLed(uint8_t ledId, int h, int s, int v) {
  if (!isValidLedId(ledId)) return false;

  // Set HSV values
  uint16_t hue16 = map(constrain(h, 0, 255), 0, 255, 0, 65535);
  uint8_t sat = constrain(s, 0, 255);
  uint8_t val = constrain(v, 0, 255);
  uint32_t rgb = Adafruit_NeoPixel::ColorHSV(hue16, sat, val);
  applyColorToOutput(ledId, rgb);
  
  // Debug output
  Serial.print("LED Output ");
  Serial.print(ledId);
  Serial.print(" color manually set to HSV: ");
  Serial.print(h); Serial.print(", ");
  Serial.print(s); Serial.print(", ");
  Serial.println(v);
  return true;
}

// New function for brightness control
void setBrightness(int brightness) {
  setBrightnessForLed(activeLedOutput, brightness);
}

bool setBrightnessForLed(uint8_t ledId, int brightness) {
  if (!isValidLedId(ledId)) return false;

  const int idx = ledId - 1;
  brightnessPerOutput[idx] = constrain(brightness, 0, 255);
  if (strips[idx] != nullptr) {
    strips[idx]->setBrightness(brightnessPerOutput[idx]);
    strips[idx]->show();
  }
  
  // Debug output
  Serial.print("LED Output ");
  Serial.print(ledId);
  Serial.print(" brightness set to: ");
  Serial.println(brightnessPerOutput[idx]);
  return true;
}

bool setActiveLedOutput(uint8_t ledId) {
  if (!isValidLedId(ledId)) return false;
  activeLedOutput = ledId;
  return true;
}

uint8_t getActiveLedOutput() {
  return activeLedOutput;
}