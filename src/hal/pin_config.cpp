#include "pin_config.h"
#include "motor_28byj48.h"
#include "advanced_motor.h"
#include "servo_control.h"
#include "led_control.h"
#include "button_control.h"
#include "driver/gpio.h"

// Global pin configuration
PinConfiguration currentPinConfig;

// Preferences object for NVS access
Preferences preferences;

static bool isValidInputGpio(int pin) {
    return pin >= 0 && GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(pin));
}

static bool isValidOutputGpio(int pin) {
    return pin >= 0 && GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(pin));
}

static int sanitizeInputPin(int pin, int fallback) {
    return isValidInputGpio(pin) ? pin : fallback;
}

static int sanitizeOutputPin(int pin, int fallback) {
    if (isValidOutputGpio(pin)) return pin;
    if (isValidOutputGpio(fallback)) return fallback;
    return 21;
}

static int sanitizeEnablePin(int pin, int fallback) {
    if (pin == -1) return -1;
    return isValidOutputGpio(pin) ? pin : fallback;
}

static void sanitizePinConfig() {
    currentPinConfig.motor_28byj48_pin1 = sanitizeOutputPin(currentPinConfig.motor_28byj48_pin1, MOTOR_28BYJ48_DEFAULT_PIN_1);
    currentPinConfig.motor_28byj48_pin2 = sanitizeOutputPin(currentPinConfig.motor_28byj48_pin2, MOTOR_28BYJ48_DEFAULT_PIN_2);
    currentPinConfig.motor_28byj48_pin3 = sanitizeOutputPin(currentPinConfig.motor_28byj48_pin3, MOTOR_28BYJ48_DEFAULT_PIN_3);
    currentPinConfig.motor_28byj48_pin4 = sanitizeOutputPin(currentPinConfig.motor_28byj48_pin4, MOTOR_28BYJ48_DEFAULT_PIN_4);

    currentPinConfig.nema23_step_pins[0] = sanitizeOutputPin(currentPinConfig.nema23_step_pins[0], STEP_PIN);
    currentPinConfig.nema23_dir_pins[0] = sanitizeOutputPin(currentPinConfig.nema23_dir_pins[0], DIR_PIN);
    currentPinConfig.nema23_enable_pins[0] = sanitizeEnablePin(currentPinConfig.nema23_enable_pins[0], ENABLE_PIN);

    currentPinConfig.nema23_step_pins[1] = sanitizeOutputPin(currentPinConfig.nema23_step_pins[1], 35);
    currentPinConfig.nema23_dir_pins[1] = sanitizeOutputPin(currentPinConfig.nema23_dir_pins[1], 34);
    currentPinConfig.nema23_enable_pins[1] = sanitizeEnablePin(currentPinConfig.nema23_enable_pins[1], -1);

    currentPinConfig.nema23_step_pins[2] = sanitizeOutputPin(currentPinConfig.nema23_step_pins[2], 33);
    currentPinConfig.nema23_dir_pins[2] = sanitizeOutputPin(currentPinConfig.nema23_dir_pins[2], 36);
    currentPinConfig.nema23_enable_pins[2] = sanitizeEnablePin(currentPinConfig.nema23_enable_pins[2], -1);

    currentPinConfig.servo_pins[0] = sanitizeOutputPin(currentPinConfig.servo_pins[0], 48);
    currentPinConfig.servo_pins[1] = sanitizeOutputPin(currentPinConfig.servo_pins[1], 47);
    currentPinConfig.servo_pins[2] = sanitizeOutputPin(currentPinConfig.servo_pins[2], 21);

    currentPinConfig.led_pins[0] = sanitizeOutputPin(currentPinConfig.led_pins[0], LED_PIN);
    currentPinConfig.led_pins[1] = sanitizeOutputPin(currentPinConfig.led_pins[1], 39);
    currentPinConfig.led_pins[2] = sanitizeOutputPin(currentPinConfig.led_pins[2], 40);

    currentPinConfig.led_counts[0] = constrain(currentPinConfig.led_counts[0], 1, MAX_LEDS_PER_OUTPUT);
    currentPinConfig.led_counts[1] = constrain(currentPinConfig.led_counts[1], 1, MAX_LEDS_PER_OUTPUT);
    currentPinConfig.led_counts[2] = constrain(currentPinConfig.led_counts[2], 1, MAX_LEDS_PER_OUTPUT);

    currentPinConfig.button_pin = sanitizeInputPin(currentPinConfig.button_pin, BUTTON_PIN);
}

/**
 * Initializes the pin configuration
 * Loads saved values or sets defaults
 */
void initPinConfig() {
    loadPinConfig();
    
    // If no valid configuration is present, set defaults
    if (!currentPinConfig.isValid) {
        resetPinConfigToDefaults();
        savePinConfig();
    }
    
    // Apply pin configuration to the respective modules
    applyPinConfig();
}

/**
 * Loads pin configuration from NVS
 */
void loadPinConfig() {
    preferences.begin(EEPROM_NAMESPACE, false);
    
    // Check if valid configuration is present
    currentPinConfig.isValid = preferences.getBool("valid", false);
    
    if (currentPinConfig.isValid) {
        // 28BYJ-48 Motor Pins
        currentPinConfig.motor_28byj48_pin1 = preferences.getInt("m28_p1", MOTOR_28BYJ48_DEFAULT_PIN_1);
        currentPinConfig.motor_28byj48_pin2 = preferences.getInt("m28_p2", MOTOR_28BYJ48_DEFAULT_PIN_2);
        currentPinConfig.motor_28byj48_pin3 = preferences.getInt("m28_p3", MOTOR_28BYJ48_DEFAULT_PIN_3);
        currentPinConfig.motor_28byj48_pin4 = preferences.getInt("m28_p4", MOTOR_28BYJ48_DEFAULT_PIN_4);
        
        // NEMA 23 Motor Pins
        currentPinConfig.nema23_step_pins[0] = preferences.getInt("nema1_stp", STEP_PIN);
        currentPinConfig.nema23_dir_pins[0] = preferences.getInt("nema1_dir", DIR_PIN);
        currentPinConfig.nema23_enable_pins[0] = preferences.getInt("nema1_en", ENABLE_PIN);
        currentPinConfig.nema23_step_pins[1] = preferences.getInt("nema2_stp", 35);
        currentPinConfig.nema23_dir_pins[1] = preferences.getInt("nema2_dir", 34);
        currentPinConfig.nema23_enable_pins[1] = preferences.getInt("nema2_en", -1);
        currentPinConfig.nema23_step_pins[2] = preferences.getInt("nema3_stp", 33);
        currentPinConfig.nema23_dir_pins[2] = preferences.getInt("nema3_dir", 36);
        currentPinConfig.nema23_enable_pins[2] = preferences.getInt("nema3_en", -1);
        
        // Servo Pins
        currentPinConfig.servo_pins[0] = preferences.getInt("servo1_p", 48);
        currentPinConfig.servo_pins[1] = preferences.getInt("servo2_p", 47);
        currentPinConfig.servo_pins[2] = preferences.getInt("servo3_p", 21);
        
        // LED Pins and counts
        currentPinConfig.led_pins[0] = preferences.getInt("led1_p", LED_PIN);
        currentPinConfig.led_pins[1] = preferences.getInt("led2_p", 39);
        currentPinConfig.led_pins[2] = preferences.getInt("led3_p", 40);
        currentPinConfig.led_counts[0] = preferences.getInt("led1_c", 1);
        currentPinConfig.led_counts[1] = preferences.getInt("led2_c", 1);
        currentPinConfig.led_counts[2] = preferences.getInt("led3_c", 1);
        
        // Button Pin
        currentPinConfig.button_pin = preferences.getInt("btn_p", BUTTON_PIN);
        
        // WiFi Configuration
        preferences.getString("wifi_ssid", currentPinConfig.wifi_ssid, sizeof(currentPinConfig.wifi_ssid));
        preferences.getString("wifi_pass", currentPinConfig.wifi_password, sizeof(currentPinConfig.wifi_password));
        preferences.getString("wifi_host", currentPinConfig.wifi_hostname, sizeof(currentPinConfig.wifi_hostname));
        
        // If WiFi data is empty, use default values
        if (strlen(currentPinConfig.wifi_ssid) == 0) {
            strcpy(currentPinConfig.wifi_ssid, "MyNetwork");
        }
        if (strlen(currentPinConfig.wifi_password) == 0) {
            strcpy(currentPinConfig.wifi_password, "MyPassword");
        }
        if (strlen(currentPinConfig.wifi_hostname) == 0) {
            strcpy(currentPinConfig.wifi_hostname, "ESP32-IScan");
        }
        
        Serial.println("✅ Pin configuration loaded from NVS");
    } else {
        Serial.println("⚠️ No valid pin configuration found in NVS");
        resetPinConfigToDefaults();
    }

    sanitizePinConfig();
    
    preferences.end();
}

/**
 * Saves the current pin configuration to NVS
 */
void savePinConfig() {
    preferences.begin(EEPROM_NAMESPACE, false);
    
    // 28BYJ-48 Motor Pins
    preferences.putInt("m28_p1", currentPinConfig.motor_28byj48_pin1);
    preferences.putInt("m28_p2", currentPinConfig.motor_28byj48_pin2);
    preferences.putInt("m28_p3", currentPinConfig.motor_28byj48_pin3);
    preferences.putInt("m28_p4", currentPinConfig.motor_28byj48_pin4);
    
    // NEMA 23 Motor Pins
    preferences.putInt("nema1_stp", currentPinConfig.nema23_step_pins[0]);
    preferences.putInt("nema1_dir", currentPinConfig.nema23_dir_pins[0]);
    preferences.putInt("nema1_en", currentPinConfig.nema23_enable_pins[0]);
    preferences.putInt("nema2_stp", currentPinConfig.nema23_step_pins[1]);
    preferences.putInt("nema2_dir", currentPinConfig.nema23_dir_pins[1]);
    preferences.putInt("nema2_en", currentPinConfig.nema23_enable_pins[1]);
    preferences.putInt("nema3_stp", currentPinConfig.nema23_step_pins[2]);
    preferences.putInt("nema3_dir", currentPinConfig.nema23_dir_pins[2]);
    preferences.putInt("nema3_en", currentPinConfig.nema23_enable_pins[2]);
    
    // Servo Pins
    preferences.putInt("servo1_p", currentPinConfig.servo_pins[0]);
    preferences.putInt("servo2_p", currentPinConfig.servo_pins[1]);
    preferences.putInt("servo3_p", currentPinConfig.servo_pins[2]);
    
    // LED Pins and counts
    preferences.putInt("led1_p", currentPinConfig.led_pins[0]);
    preferences.putInt("led2_p", currentPinConfig.led_pins[1]);
    preferences.putInt("led3_p", currentPinConfig.led_pins[2]);
    preferences.putInt("led1_c", currentPinConfig.led_counts[0]);
    preferences.putInt("led2_c", currentPinConfig.led_counts[1]);
    preferences.putInt("led3_c", currentPinConfig.led_counts[2]);
    
    // Button Pin
    preferences.putInt("btn_p", currentPinConfig.button_pin);
    
    // WiFi Configuration
    preferences.putString("wifi_ssid", currentPinConfig.wifi_ssid);
    preferences.putString("wifi_pass", currentPinConfig.wifi_password);
    preferences.putString("wifi_host", currentPinConfig.wifi_hostname);
    
    // Set validation flag
    preferences.putBool("valid", true);
    currentPinConfig.isValid = true;
    
    preferences.end();
}

/**
 * Resets pin configuration to defaults
 */
void resetPinConfigToDefaults() {
    // 28BYJ-48 Motor Pins
    currentPinConfig.motor_28byj48_pin1 = MOTOR_28BYJ48_DEFAULT_PIN_1;
    currentPinConfig.motor_28byj48_pin2 = MOTOR_28BYJ48_DEFAULT_PIN_2;
    currentPinConfig.motor_28byj48_pin3 = MOTOR_28BYJ48_DEFAULT_PIN_3;
    currentPinConfig.motor_28byj48_pin4 = MOTOR_28BYJ48_DEFAULT_PIN_4;
    
    // NEMA 23 Motor Pins
    currentPinConfig.nema23_step_pins[0] = STEP_PIN;
    currentPinConfig.nema23_dir_pins[0] = DIR_PIN;
    currentPinConfig.nema23_enable_pins[0] = ENABLE_PIN;
    currentPinConfig.nema23_step_pins[1] = 35;
    currentPinConfig.nema23_dir_pins[1] = 34;
    currentPinConfig.nema23_enable_pins[1] = -1;
    currentPinConfig.nema23_step_pins[2] = 33;
    currentPinConfig.nema23_dir_pins[2] = 36;
    currentPinConfig.nema23_enable_pins[2] = -1;
    
    // Servo Pins
    currentPinConfig.servo_pins[0] = 48;
    currentPinConfig.servo_pins[1] = 47;
    currentPinConfig.servo_pins[2] = 21;
    
    // LED Pins and counts
    currentPinConfig.led_pins[0] = LED_PIN;
    currentPinConfig.led_pins[1] = 39;
    currentPinConfig.led_pins[2] = 40;
    currentPinConfig.led_counts[0] = 1;
    currentPinConfig.led_counts[1] = 1;
    currentPinConfig.led_counts[2] = 1;
    
    // Button Pin
    currentPinConfig.button_pin = BUTTON_PIN;
    
    // WiFi Configuration - Defaults
    strncpy(currentPinConfig.wifi_ssid, "MyNetwork", sizeof(currentPinConfig.wifi_ssid));
    strncpy(currentPinConfig.wifi_password, "MyPassword", sizeof(currentPinConfig.wifi_password));
    strncpy(currentPinConfig.wifi_hostname, "ESP32-IScan", sizeof(currentPinConfig.wifi_hostname));
    
    currentPinConfig.isValid = true;
    
    Serial.println("🔄 Pin configuration reset to defaults");
}

/**
 * Applies the current pin configuration to all modules
 */
void applyPinConfig() {
    sanitizePinConfig();

    // Set 28BYJ-48 Motor Pins
    motor_28byj48_pin_1 = currentPinConfig.motor_28byj48_pin1;
    motor_28byj48_pin_2 = currentPinConfig.motor_28byj48_pin2;
    motor_28byj48_pin_3 = currentPinConfig.motor_28byj48_pin3;
    motor_28byj48_pin_4 = currentPinConfig.motor_28byj48_pin4;
    
    // Set servo pins
    for (int i = 0; i < MAX_SERVOS; i++) {
        SERVO_GPIO_PINS[i] = currentPinConfig.servo_pins[i];
    }
    SERVO_GPIO_PIN = SERVO_GPIO_PINS[0];

    // Set NEMA23 pins
    for (int i = 0; i < MAX_ADVANCED_MOTORS; i++) {
        ADV_MOTOR_STEP_PINS[i] = currentPinConfig.nema23_step_pins[i];
        ADV_MOTOR_DIR_PINS[i] = currentPinConfig.nema23_dir_pins[i];
        ADV_MOTOR_ENABLE_PINS[i] = currentPinConfig.nema23_enable_pins[i];
    }

    // Set LED pins and counts
    for (int i = 0; i < MAX_LED_OUTPUTS; i++) {
        LED_GPIO_PINS[i] = currentPinConfig.led_pins[i];
        LED_COUNTS[i] = currentPinConfig.led_counts[i];
    }
    
    Serial.println("✅ Pin configuration applied to modules");
}

/**
 * Sets the complete pin configuration
 */
void setPinConfig(const PinConfiguration& config) {
    currentPinConfig = config;
    currentPinConfig.isValid = true;
    applyPinConfig();
}

/**
 * Returns the current pin configuration
 */
PinConfiguration getPinConfig() {
    return currentPinConfig;
}

/**
 * Prints the pin configuration to the serial console
 */
void printPinConfig() {
    Serial.println("\n[PIN CONFIG] Current pin configuration:");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.printf("28BYJ-48 Motor: IN1=%d, IN2=%d, IN3=%d, IN4=%d\n", 
                  currentPinConfig.motor_28byj48_pin1,
                  currentPinConfig.motor_28byj48_pin2,
                  currentPinConfig.motor_28byj48_pin3,
                  currentPinConfig.motor_28byj48_pin4);
    Serial.printf("NEMA23: M1 STEP=%d DIR=%d EN=%d | M2 STEP=%d DIR=%d EN=%d | M3 STEP=%d DIR=%d EN=%d\n",
                  currentPinConfig.nema23_step_pins[0], currentPinConfig.nema23_dir_pins[0], currentPinConfig.nema23_enable_pins[0],
                  currentPinConfig.nema23_step_pins[1], currentPinConfig.nema23_dir_pins[1], currentPinConfig.nema23_enable_pins[1],
                  currentPinConfig.nema23_step_pins[2], currentPinConfig.nema23_dir_pins[2], currentPinConfig.nema23_enable_pins[2]);
    Serial.printf("Servo: S1=%d, S2=%d, S3=%d\n",
                  currentPinConfig.servo_pins[0],
                  currentPinConfig.servo_pins[1],
                  currentPinConfig.servo_pins[2]);
    Serial.printf("LED: O1 GPIO %d x%d, O2 GPIO %d x%d, O3 GPIO %d x%d\n",
                  currentPinConfig.led_pins[0], currentPinConfig.led_counts[0],
                  currentPinConfig.led_pins[1], currentPinConfig.led_counts[1],
                  currentPinConfig.led_pins[2], currentPinConfig.led_counts[2]);
    Serial.printf("Button: GPIO %d\n", currentPinConfig.button_pin);
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// Individual pin setters
void set28BYJ48Pins(int pin1, int pin2, int pin3, int pin4) {
    // Update config struct
    currentPinConfig.motor_28byj48_pin1 = pin1;
    currentPinConfig.motor_28byj48_pin2 = pin2;
    currentPinConfig.motor_28byj48_pin3 = pin3;
    currentPinConfig.motor_28byj48_pin4 = pin4;
    
    // Update global variables
    motor_28byj48_pin_1 = pin1;
    motor_28byj48_pin_2 = pin2;
    motor_28byj48_pin_3 = pin3;
    motor_28byj48_pin_4 = pin4;
    
    // Save to NVS
    savePinConfig();
    
    Serial.printf("✓ 28BYJ-48 pins saved: %d, %d, %d, %d\n", pin1, pin2, pin3, pin4);
}

void setNEMA23Pins(int stepPin, int dirPin, int enablePin) {
    currentPinConfig.nema23_step_pins[0] = stepPin;
    currentPinConfig.nema23_dir_pins[0] = dirPin;
    currentPinConfig.nema23_enable_pins[0] = enablePin;
    ADV_MOTOR_STEP_PINS[0] = stepPin;
    ADV_MOTOR_DIR_PINS[0] = dirPin;
    ADV_MOTOR_ENABLE_PINS[0] = enablePin;
    savePinConfig();
}

void setNEMA23PinsById(int motorId, int stepPin, int dirPin, int enablePin) {
    if (motorId < 1 || motorId > MAX_ADVANCED_MOTORS) {
        return;
    }
    currentPinConfig.nema23_step_pins[motorId - 1] = stepPin;
    currentPinConfig.nema23_dir_pins[motorId - 1] = dirPin;
    currentPinConfig.nema23_enable_pins[motorId - 1] = enablePin;
    ADV_MOTOR_STEP_PINS[motorId - 1] = stepPin;
    ADV_MOTOR_DIR_PINS[motorId - 1] = dirPin;
    ADV_MOTOR_ENABLE_PINS[motorId - 1] = enablePin;
    savePinConfig();
}

void setServoPin(int pin) {
    currentPinConfig.servo_pins[0] = pin;
    SERVO_GPIO_PINS[0] = pin;
    SERVO_GPIO_PIN = pin;
    savePinConfig();
}

void setServoPinById(int servoId, int pin) {
    if (servoId < 1 || servoId > MAX_SERVOS) {
        return;
    }
    currentPinConfig.servo_pins[servoId - 1] = pin;
    SERVO_GPIO_PINS[servoId - 1] = pin;
    SERVO_GPIO_PIN = SERVO_GPIO_PINS[0];
    savePinConfig();
}

void setLedPin(int pin) {
    currentPinConfig.led_pins[0] = pin;
    LED_GPIO_PINS[0] = pin;
    savePinConfig();
}

void setLedPinById(int ledId, int pin) {
    if (ledId < 1 || ledId > MAX_LED_OUTPUTS) {
        return;
    }
    currentPinConfig.led_pins[ledId - 1] = pin;
    LED_GPIO_PINS[ledId - 1] = pin;
    savePinConfig();
}

void setLedCountById(int ledId, int count) {
    if (ledId < 1 || ledId > MAX_LED_OUTPUTS) {
        return;
    }
    currentPinConfig.led_counts[ledId - 1] = constrain(count, 1, MAX_LEDS_PER_OUTPUT);
    LED_COUNTS[ledId - 1] = currentPinConfig.led_counts[ledId - 1];
    savePinConfig();
}

void setButtonPin(int pin) {
    currentPinConfig.button_pin = pin;
    savePinConfig();
}

void setWiFiConfig(const char* ssid, const char* password, const char* hostname) {
    strncpy(currentPinConfig.wifi_ssid, ssid, sizeof(currentPinConfig.wifi_ssid) - 1);
    currentPinConfig.wifi_ssid[sizeof(currentPinConfig.wifi_ssid) - 1] = '\0';
    
    strncpy(currentPinConfig.wifi_password, password, sizeof(currentPinConfig.wifi_password) - 1);
    currentPinConfig.wifi_password[sizeof(currentPinConfig.wifi_password) - 1] = '\0';
    
    strncpy(currentPinConfig.wifi_hostname, hostname, sizeof(currentPinConfig.wifi_hostname) - 1);
    currentPinConfig.wifi_hostname[sizeof(currentPinConfig.wifi_hostname) - 1] = '\0';
    
    savePinConfig();
}

// Einzelne Pin-Getter
void get28BYJ48Pins(int* pin1, int* pin2, int* pin3, int* pin4) {
    *pin1 = currentPinConfig.motor_28byj48_pin1;
    *pin2 = currentPinConfig.motor_28byj48_pin2;
    *pin3 = currentPinConfig.motor_28byj48_pin3;
    *pin4 = currentPinConfig.motor_28byj48_pin4;
}

void getNEMA23Pins(int* stepPin, int* dirPin, int* enablePin) {
    *stepPin = currentPinConfig.nema23_step_pins[0];
    *dirPin = currentPinConfig.nema23_dir_pins[0];
    *enablePin = currentPinConfig.nema23_enable_pins[0];
}

void getNEMA23PinsById(int motorId, int* stepPin, int* dirPin, int* enablePin) {
    if (motorId < 1 || motorId > MAX_ADVANCED_MOTORS) {
        if (stepPin) *stepPin = -1;
        if (dirPin) *dirPin = -1;
        if (enablePin) *enablePin = -1;
        return;
    }
    if (stepPin) *stepPin = currentPinConfig.nema23_step_pins[motorId - 1];
    if (dirPin) *dirPin = currentPinConfig.nema23_dir_pins[motorId - 1];
    if (enablePin) *enablePin = currentPinConfig.nema23_enable_pins[motorId - 1];
}

int getServoPin() {
    return currentPinConfig.servo_pins[0];
}

int getServoPinById(int servoId) {
    if (servoId < 1 || servoId > MAX_SERVOS) {
        return -1;
    }
    return currentPinConfig.servo_pins[servoId - 1];
}

int getLedPin() {
    return currentPinConfig.led_pins[0];
}

int getLedPinById(int ledId) {
    if (ledId < 1 || ledId > MAX_LED_OUTPUTS) {
        return -1;
    }
    return currentPinConfig.led_pins[ledId - 1];
}

int getLedCountById(int ledId) {
    if (ledId < 1 || ledId > MAX_LED_OUTPUTS) {
        return -1;
    }
    return currentPinConfig.led_counts[ledId - 1];
}

int getButtonPin() {
    return currentPinConfig.button_pin;
}

void getWiFiConfig(char* ssid, char* password, char* hostname) {
    if (ssid) strcpy(ssid, currentPinConfig.wifi_ssid);
    if (password) strcpy(password, currentPinConfig.wifi_password);
    if (hostname) strcpy(hostname, currentPinConfig.wifi_hostname);
}

// ==================== Device Information Functions ====================

// Globale Device Information
DeviceInfo currentDeviceInfo;

/**
 * Initializes device information
 * Loads saved values or sets defaults
 */
void initDeviceInfo() {
    loadDeviceInfo();
}

/**
 * Loads device information from NVS
 */
void loadDeviceInfo() {
    preferences.begin(DEVICE_INFO_NAMESPACE, false);
    
    currentDeviceInfo.deviceName = preferences.getString("deviceName", "D-ActorNode");
    currentDeviceInfo.deviceNumber = preferences.getString("deviceNumber", "0001");
    currentDeviceInfo.configuration = preferences.getString("configuration", "Default Configuration");
    currentDeviceInfo.description = preferences.getString("description", "");
    
    preferences.end();
    
    Serial.println("✅ Device information loaded from NVS");
    Serial.println("   Device Name: " + currentDeviceInfo.deviceName);
    Serial.println("   Device Number: " + currentDeviceInfo.deviceNumber);
    Serial.println("   Configuration: " + currentDeviceInfo.configuration);
    Serial.println("   Description: " + currentDeviceInfo.description);
}

/**
 * Speichert die aktuelle Device Information im EEPROM
 */
void saveDeviceInfo() {
    preferences.begin(DEVICE_INFO_NAMESPACE, false);
    
    preferences.putString("deviceName", currentDeviceInfo.deviceName);
    preferences.putString("deviceNumber", currentDeviceInfo.deviceNumber);
    preferences.putString("configuration", currentDeviceInfo.configuration);
    preferences.putString("description", currentDeviceInfo.description);
    
    preferences.end();
}

/**
 * Returns the current device information
 */
DeviceInfo getDeviceInfo() {
    return currentDeviceInfo;
}

/**
 * Sets the device name
 */
void setDeviceName(const char* name) {
    currentDeviceInfo.deviceName = String(name);
    saveDeviceInfo();
}

/**
 * Sets the device number
 */
void setDeviceNumber(const char* number) {
    currentDeviceInfo.deviceNumber = String(number);
    saveDeviceInfo();
}

/**
 * Sets the configuration
 */
void setConfiguration(const char* config) {
    currentDeviceInfo.configuration = String(config);
    saveDeviceInfo();
}

/**
 * Sets the description
 */
void setDescription(const char* desc) {
    currentDeviceInfo.description = String(desc);
    saveDeviceInfo();
}

