#include "wifi_manager.h"
#include "pin_config.h"
#include <cstring>

// Network configuration - loaded from NVS
char current_ssid[64] = "";
char current_password[64] = "";
char current_hostname[32] = "D-ActorNode";

// AP mode status
bool wifi_ap_mode_active = false;

// Interval for re-attempting STA connection in AP mode (ms)
static const unsigned long AP_RETRY_INTERVAL_MS = 60000UL;
static unsigned long lastApRetryMs = 0;

namespace {
const char DEFAULT_WIFI_HOSTNAME[] = "D-ActorNode";
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

void copyWithFallback(char* destination, size_t destinationSize, const char* value, const char* fallback) {
  if (destinationSize == 0) return;
  const char* source = (value && value[0] != '\0') ? value : fallback;
  strncpy(destination, source, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

bool connectToWiFi(const char* ssid, const char* password, unsigned long timeoutMs) {
  if (!ssid || ssid[0] == '\0') {
    Serial.println("[WiFi] No SSID configured - connection skipped.");
    return false;
  }

  Serial.print("[WiFi] Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, (password && password[0] != '\0') ? password : nullptr);

  const unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  return WiFi.status() == WL_CONNECTED;
}
} // namespace

// ─── Access-Point starten ─────────────────────────────────────────────────────

void startAccessPoint() {
  DeviceInfo info = getDeviceInfo();
  const char* apSsid = info.deviceName.c_str();

  Serial.printf("[WiFi] Starting Access Point: \"%s\" (no password)\n", apSsid);

  WiFi.disconnect(true);
  delay(200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid);   // open network

  delay(200);
  IPAddress apIp = WiFi.softAPIP();
  Serial.print("[WiFi] AP started - IP: ");
  Serial.println(apIp);
  Serial.println("[WiFi] Web interface available at: http://" + apIp.toString());

  wifi_ap_mode_active = true;
  lastApRetryMs = millis();

  setColorByIndex(2);  // Yellow/Orange = AP mode
}

// ─── setupWiFi ────────────────────────────────────────────────────────────────

void setupWiFi() {
  // Load WiFi configuration from NVS
  getWiFiConfig(current_ssid, current_password, current_hostname);

  // Fallback-Hostname
  copyWithFallback(current_hostname, sizeof(current_hostname), current_hostname, DEFAULT_WIFI_HOSTNAME);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);  // We manage reconnect ourselves
  WiFi.persistent(false);
  WiFi.setHostname(current_hostname);

  Serial.printf("[WiFi] Hostname: %s\n", current_hostname);

  bool isConnected = false;

  // Connection attempt with saved SSID (only if set)
  if (current_ssid[0] != '\0') {
    isConnected = connectToWiFi(current_ssid, current_password, WIFI_CONNECT_TIMEOUT_MS);
  }

  if (isConnected) {
    wifi_ap_mode_active = false;
    printNetworkStatus();
    setColorByIndex(1);  // Green = connected
  } else {
    Serial.println("[WiFi] Connection failed - starting own Access Point.");
    startAccessPoint();
  }
}

// ─── checkWiFiConnection ─────────────────────────────────────────────────────

void checkWiFiConnection() {
  if (wifi_ap_mode_active) {
    // In AP mode: periodically attempt STA connection
    if (millis() - lastApRetryMs >= AP_RETRY_INTERVAL_MS) {
      lastApRetryMs = millis();

      if (current_ssid[0] == '\0') return;  // no SSID -> skip

      Serial.println("[WiFi] AP mode: attempting STA connection...");
      WiFi.mode(WIFI_STA);
      WiFi.setHostname(current_hostname);

      bool isConnected = connectToWiFi(current_ssid, current_password, 10000);
      if (isConnected) {
        wifi_ap_mode_active = false;
        printNetworkStatus();
        setColorByIndex(1);
      } else {
        Serial.println("[WiFi] STA attempt failed - staying in AP mode.");
        WiFi.disconnect(true);
        delay(200);
        WiFi.mode(WIFI_AP);
        DeviceInfo info = getDeviceInfo();
        WiFi.softAP(info.deviceName.c_str());
        delay(200);
      }
    }
    return;
  }

  // STA mode: reconnect on connection loss, then AP as fallback
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost. Attempting reconnect...");
    setColorByIndex(0);  // Rot

    bool isConnected = connectToWiFi(current_ssid, current_password, 8000);

    if (isConnected) {
      printNetworkStatus();
      setColorByIndex(1);
    } else {
      Serial.println("[WiFi] Reconnect failed - starting Access Point.");
      startAccessPoint();
    }
  }
}

// ─── printNetworkStatus ───────────────────────────────────────────────────────

void printNetworkStatus() {
  Serial.print("[WiFi] Connected to: ");
  Serial.println(WiFi.SSID());
  Serial.print("[WiFi] Local IP:      ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] Hostname:      ");
  Serial.println(WiFi.getHostname());
}