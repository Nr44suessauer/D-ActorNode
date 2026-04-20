#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "led_control.h"

extern const uint16_t HTTP_PORT;
extern WebServer server;

void setupWebServer();
void handleWebServerRequests();

// Handlers for various routes
void handleRoot();
void handleColorChange();
void handleHexColorChange();
void handleNotFound();
void handleGetButtonState(); // New handler for button status
void handleBrightness(); // Handler for LED brightness control

// Extended motor control handlers
void handleMotorControl();
void handleMotorStop();
void handleMotorHome();
void handleMotorStatus();
void handleMotorCalibrate();
void handleMotorDegrees();

#endif // WEB_SERVER_H