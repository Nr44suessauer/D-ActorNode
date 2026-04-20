#ifndef UART_WIFI_CONFIG_H
#define UART_WIFI_CONFIG_H

#include <Arduino.h>

/**
 * Initializes the UART-WiFi configuration.
 * Prints a help message to the serial console.
 */
void setupUartWifiConfig();

/**
 * Reads serial input and processes WiFi configuration commands.
 * Must be called in every loop() iteration.
 */
void handleUartWifiConfig();

#endif // UART_WIFI_CONFIG_H
