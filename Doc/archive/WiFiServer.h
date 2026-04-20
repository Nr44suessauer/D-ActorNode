#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>
#include "Server.h"
#include "WiFiClient.h"

class WiFiServer : public Server {
private:
  // Minimalimplementierung
  uint16_t _port;
  bool _listening;

public:
  WiFiServer(uint16_t port=80): _port(port), _listening(false) {}
  
  void begin() { _listening = true; }
  WiFiClient available() { return WiFiClient(); }
  bool hasClient() { return false; }
  void end() { _listening = false; }
  operator bool() { return _listening; }
};

#endif // WIFI_SERVER_H