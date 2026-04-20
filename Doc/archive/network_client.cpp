#include "network_client.h"

// TCP Server-Konfiguration
const IPAddress SERVER_IP(192, 168, 1, 100); // Anpassen an die tatsächliche IP
const uint16_t TCP_PORT = 5000;             // Anpassen an den tatsächlichen Port
WiFiClient client;
bool isConnected = false;

// Timing-Konfiguration
const unsigned long RECONNECT_CHECK_INTERVAL = 1000;  // 1 Sekunde
const unsigned long HEARTBEAT_INTERVAL = 10000;       // 10 Sekunden

// Status-Tracking
unsigned long lastHeartbeat = 0;
unsigned long lastReconnectAttempt = 0;

bool connectToServer() {
  Serial.printf("Verbindung zum Server %s:%d wird hergestellt...\n", 
                SERVER_IP.toString().c_str(), TCP_PORT);
  
  if (client.connect(SERVER_IP, TCP_PORT)) {
    Serial.println("Verbindung hergestellt!");
    isConnected = true;
    return true;
  }
  
  Serial.println("Verbindung fehlgeschlagen");
  isConnected = false;
  return false;
}

bool sendHeartbeat() {
  if (!isConnected) {
    return false;
  }
  
  char message[64];
  snprintf(message, sizeof(message), "Client heartbeat from %s", 
           WiFi.localIP().toString().c_str());
  
  if (client.println(message)) {
    Serial.println("Heartbeat gesendet");
    return true;
  }
  
  Serial.println("Fehler beim Senden des Heartbeats");
  isConnected = false;
  return false;
}

void checkServerConnection() {
  unsigned long currentTime = millis();
  
  // Prüfen, ob Daten vom Server verfügbar sind
  if (isConnected && client.available()) {
    String data = client.readStringUntil('\n');
    Serial.print("Vom Server empfangen: ");
    Serial.println(data);
  }
  
  // Heartbeat-Logik
  if (isConnected && (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL)) {
    if (sendHeartbeat()) {
      lastHeartbeat = currentTime;
    }
  }
  
  // Verbindungswiederherstellung
  if (!isConnected && (currentTime - lastReconnectAttempt >= RECONNECT_CHECK_INTERVAL)) {
    connectToServer();
    lastReconnectAttempt = currentTime;
  }
}