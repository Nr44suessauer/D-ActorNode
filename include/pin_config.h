#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include "advanced_motor.h"
#include "servo_control.h"
#include "led_control.h"

// EEPROM Namespace
#define EEPROM_NAMESPACE "pin_config"
#define DEVICE_INFO_NAMESPACE "device_info"

// Pin configuration structure
struct PinConfiguration {
    // 28BYJ-48 Motor Pins
    int motor_28byj48_pin1;
    int motor_28byj48_pin2;
    int motor_28byj48_pin3;
    int motor_28byj48_pin4;
    
    // NEMA 23 Motor Pins (up to 3 motors)
    int nema23_step_pins[MAX_ADVANCED_MOTORS];
    int nema23_dir_pins[MAX_ADVANCED_MOTORS];
    int nema23_enable_pins[MAX_ADVANCED_MOTORS];
    
    // Servo Pins (up to 3 independent servos)
    int servo_pins[MAX_SERVOS];
    
    // LED Pins and counts (up to 3 outputs)
    int led_pins[MAX_LED_OUTPUTS];
    int led_counts[MAX_LED_OUTPUTS];
    
    // Button Pin
    int button_pin;
    
    // WiFi Configuration
    char wifi_ssid[64];
    char wifi_password[64];
    char wifi_hostname[32];
    
    // Validierungs-Flag
    bool isValid;
};

// Global pin configuration
extern PinConfiguration currentPinConfig;

// Funktionsprototypen
void initPinConfig();
void loadPinConfig();
void savePinConfig();
void resetPinConfigToDefaults();
void applyPinConfig();
void setPinConfig(const PinConfiguration& config);
PinConfiguration getPinConfig();
void printPinConfig();

// Einzelne Pin-Setter
void set28BYJ48Pins(int pin1, int pin2, int pin3, int pin4);
void setNEMA23Pins(int stepPin, int dirPin, int enablePin);
void setNEMA23PinsById(int motorId, int stepPin, int dirPin, int enablePin);
void setServoPin(int pin);
void setServoPinById(int servoId, int pin);
void setLedPin(int pin);
void setLedPinById(int ledId, int pin);
void setLedCountById(int ledId, int count);
void setButtonPin(int pin);
void setWiFiConfig(const char* ssid, const char* password, const char* hostname);

// Einzelne Pin-Getter
void get28BYJ48Pins(int* pin1, int* pin2, int* pin3, int* pin4);
void getNEMA23Pins(int* stepPin, int* dirPin, int* enablePin);
void getNEMA23PinsById(int motorId, int* stepPin, int* dirPin, int* enablePin);
int getServoPin();
int getServoPinById(int servoId);
int getLedPin();
int getLedPinById(int ledId);
int getLedCountById(int ledId);
int getButtonPin();
void getWiFiConfig(char* ssid, char* password, char* hostname);

// Device Information Struktur
struct DeviceInfo {
    String deviceName;
    String deviceNumber;
    String configuration;
    String description;
};

// Device Information Funktionen
void initDeviceInfo();
void loadDeviceInfo();
void saveDeviceInfo();
DeviceInfo getDeviceInfo();
void setDeviceName(const char* name);
void setDeviceNumber(const char* number);
void setConfiguration(const char* config);
void setDescription(const char* desc);

#endif // PIN_CONFIG_H
