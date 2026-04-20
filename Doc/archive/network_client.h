#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>

// TCP Server-Konfiguration
extern const IPAddress SERVER_IP;
extern const uint16_t TCP_PORT;
extern WiFiClient client;
extern bool isConnected;

// Timing-Konfiguration
extern const unsigned long RECONNECT_CHECK_INTERVAL;
extern const unsigned long HEARTBEAT_INTERVAL;

// Status-Tracking
extern unsigned long lastHeartbeat;
extern unsigned long lastReconnectAttempt;

bool connectToServer();
bool sendHeartbeat();
void checkServerConnection();

#endif // NETWORK_CLIENT_H