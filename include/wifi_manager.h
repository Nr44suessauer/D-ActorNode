#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "led_control.h"

// Network Configuration - loaded from EEPROM
extern char current_ssid[64];
extern char current_password[64];
extern char current_hostname[32];

// AP mode status
extern bool wifi_ap_mode_active;

void setupWiFi();
void checkWiFiConnection();
void printNetworkStatus();

/**
 * Startet den ESP32 als Access Point.
 * Der AP-Name wird aus dem DeviceInfo.deviceName gelesen.
 * Das Netzwerk ist offen (kein Passwort).
 */
void startAccessPoint();

#endif // WIFI_MANAGER_H