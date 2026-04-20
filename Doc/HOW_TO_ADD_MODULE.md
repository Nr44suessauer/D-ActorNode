# Wie füge ich ein neues Hardware-Modul hinzu?

Diese Anleitung beschreibt den vollständigen Weg — vom Treiber bis zur REST-API und UART-Schnittstelle — am Beispiel eines fiktiven Distanzsensors `hcsr04`.

---

## Projektstruktur (Übersicht)

```
src/                        ← Gesamter Projektquellcode
│   main.cpp
│   drivers/                ← Hardware-Treiber (.cpp)
│   │   motor_28byj48.cpp
│   │   led_control.cpp
│   │   button_control.cpp
│   │   servo_control.cpp
│   │   ...
│   hal/                    ← Hardware Abstraction Layer (Pin-Konfiguration)
│   │   pin_config.cpp
│   wifi/                   ← WiFi-Manager
│   │   wifi_manager.cpp
│   api/                    ← REST-API (Routen & Handler)
│   │   web_server.cpp
│   uart/                   ← UART-Kommandointerface
│   │   uart_wifi_config.cpp
│   network/                ← Netzwerk-Client
│       network_client.cpp
│
include/                    ← Alle Header-Dateien (flach)
│   pin_config.h
│   motor_28byj48.h
│   led_control.h
│   wifi_manager.h
│   web_server.h
│   uart_wifi_config.h
│   ...
│
Doc/                        ← Dokumentation
diagrams/                   ← PlantUML-Diagramme
```

**Faustregel:**
- Hardware-Treiber (`.cpp`) → `src/drivers/`
- Treiber-Header (`.h`) → `include/` (flach, keine Unterordner)
- Systemcode (WiFi, HTTP, UART, HAL) → `src/<subsystem>/`
- Treiber-Header in `include/` sind bewusst flach gehalten, da PlatformIO `include/` beim Build-Scan nur auf der Wurzelebene verarbeitet

---

## Schritt 1 – Treiber anlegen

Lege die Treiber-Dateien an. Name = Gerät/Chip-Bezeichnung in Kleinbuchstaben.

```
src/drivers/hcsr04.cpp
include/hcsr04.h
```

### `include/hcsr04.h`

```cpp
#ifndef HCSR04_H
#define HCSR04_H

#include <Arduino.h>

// Konfiguration
#define HCSR04_DEFAULT_TRIG_PIN  10
#define HCSR04_DEFAULT_ECHO_PIN  11

// Initialisierung
void setupHCSR04(int trigPin, int echoPin);

// Messung
float hcsr04MeasureCm();

// Status-Typ
typedef struct {
    float lastDistanceCm;
    bool  sensorReady;
} HCSR04Status;

HCSR04Status getHCSR04Status();

#endif // HCSR04_H
```

**Konventionen für Treiber-Header:**
- Guard-Makro: `#ifndef <NAME>_H`
- Nur `<Arduino.h>` und gerätespezifische Systemincludes
- Keine Abhängigkeiten zu anderen Projektmodulen
- Status-Struct für Statusabfrage (wird von API und UART genutzt)
- Funktionspräfix = Gerätename (`hcsr04...`)

### `src/drivers/hcsr04.cpp`

```cpp
#include "hcsr04.h"

static int _trigPin = HCSR04_DEFAULT_TRIG_PIN;
static int _echoPin = HCSR04_DEFAULT_ECHO_PIN;
static HCSR04Status _status = { 0.0f, false };

void setupHCSR04(int trigPin, int echoPin) {
    _trigPin = trigPin;
    _echoPin = echoPin;
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    _status.sensorReady = true;
}

float hcsr04MeasureCm() {
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    long duration = pulseIn(_echoPin, HIGH, 30000UL);
    float dist = duration * 0.0343f / 2.0f;
    _status.lastDistanceCm = dist;
    return dist;
}

HCSR04Status getHCSR04Status() {
    return _status;
}
```

PlatformIO kompiliert alle `.cpp`-Dateien in `src/` automatisch — keine Eintragung in `platformio.ini` nötig.

---

## Schritt 2 – Pins registrieren (`include/pin_config.h`)

Füge die neuen Pins zur `PinConfiguration`-Struct hinzu:

```cpp
// include/pin_config.h  →  struct PinConfiguration
int hcsr04_trig_pin;   // NEU
int hcsr04_echo_pin;   // NEU
```

Trage Standardwerte in `src/hal/pin_config.cpp` → `initPinConfig()` ein:

```cpp
// src/hal/pin_config.cpp  →  initPinConfig()
currentPinConfig.hcsr04_trig_pin = HCSR04_DEFAULT_TRIG_PIN;
currentPinConfig.hcsr04_echo_pin = HCSR04_DEFAULT_ECHO_PIN;
```

---

## Schritt 3 – main.cpp verdrahten (`src/main.cpp`)

```cpp
// Treiber einbinden
#include "hcsr04.h"

void setup() {
    // ... andere setup()-Aufrufe ...
    setupHCSR04(currentPinConfig.hcsr04_trig_pin,
                currentPinConfig.hcsr04_echo_pin);
}
```

---

## Schritt 4 – REST-API Routen (`src/api/web_server.cpp`)

### 4a – Handler-Deklaration in `include/web_server.h`

```cpp
void handleHCSR04Status();   // NEU
void handleHCSR04Measure();  // NEU
```

### 4b – Handler implementieren in `src/api/web_server.cpp`

```cpp
#include "hcsr04.h"   // NEU oben hinzufügen

// Handler: GET /hcsr04/status
void handleHCSR04Status() {
    HCSR04Status s = getHCSR04Status();
    String json = "{";
    json += "\"ready\":" + String(s.sensorReady ? "true" : "false") + ",";
    json += "\"lastDistanceCm\":" + String(s.lastDistanceCm, 1);
    json += "}";
    server.send(200, "application/json", json);
}

// Handler: GET /hcsr04/measure
void handleHCSR04Measure() {
    float dist = hcsr04MeasureCm();
    String json = "{\"distanceCm\":" + String(dist, 1) + "}";
    server.send(200, "application/json", json);
}
```

### 4c – Routen registrieren in `setupWebServer()`

```cpp
void setupWebServer() {
    // ... bestehende Routen ...

    // HC-SR04
    server.on("/hcsr04/status",  HTTP_GET, handleHCSR04Status);
    server.on("/hcsr04/measure", HTTP_GET, handleHCSR04Measure);
}
```

**API-Konventionen:**
| Methode | Pfad-Muster | Zweck |
|---|---|---|
| GET | `/hcsr04/status` | Status / Konfiguration lesen |
| GET | `/hcsr04/measure` | Messung auslösen & Ergebnis |
| POST | `/hcsr04/config` | Parameter setzen |

---

## Schritt 5 – UART-Befehle (`src/uart/uart_wifi_config.cpp`)

### 5a – Hilfetexttabelle erweitern in `printHelp()`

```cpp
Serial.println("╠════════════════════════════════════════════════════════════╣");
Serial.println("║  hcsr04 measure          Misst Abstand in cm              ║");
Serial.println("║  hcsr04 status           Zeigt Sensor-Status              ║");
```

### 5b – Befehlsprozessor hinzufügen (vor `processCommand()`)

```cpp
#include "hcsr04.h"   // NEU am Dateianfang

static void processHCSR04Command(const char* cmd) {

    if (strcasecmp(cmd, "measure") == 0) {
        float dist = hcsr04MeasureCm();
        Serial.printf("✓ Abstand: %.1f cm\n", dist);
        return;
    }

    if (strcasecmp(cmd, "status") == 0) {
        HCSR04Status s = getHCSR04Status();
        Serial.println("\n[HC-SR04 Status]");
        Serial.printf("  Bereit          : %s\n", s.sensorReady ? "Ja" : "Nein");
        Serial.printf("  Letzter Wert    : %.1f cm\n", s.lastDistanceCm);
        return;
    }

    Serial.printf("[Fehler] Unbekannter hcsr04-Befehl: 'hcsr04 %s'\n", cmd);
    printHelp();
}
```

### 5c – Dispatcher eintragen in `processCommand()`

```cpp
static void processCommand(const char* line) {
    // ... bestehende Dispatcher ...

    if (strncasecmp(line, "hcsr04 ", 7) == 0) {
        processHCSR04Command(line + 7);
        return;
    }
}
```

---

## Checkliste (Zusammenfassung)

| Schritt | Datei | Was tun |
|---|---|---|
| 1 | `include/hcsr04.h` | Treiber-Header anlegen |
| 1 | `src/drivers/hcsr04.cpp` | Treiber implementieren |
| 2 | `include/pin_config.h` | Pins zur Struct hinzufügen |
| 2 | `src/hal/pin_config.cpp` | Standardwerte in `initPinConfig()` |
| 3 | `src/main.cpp` | `#include` + `setup<Name>()` in `setup()` |
| 4a | `include/web_server.h` | Handler deklarieren |
| 4b | `src/api/web_server.cpp` | Handler implementieren + `#include` |
| 4c | `src/api/web_server.cpp` | Route in `setupWebServer()` eintragen |
| 5a | `src/uart/uart_wifi_config.cpp` | Hilfetexttabelle erweitern |
| 5b | `src/uart/uart_wifi_config.cpp` | `processXxxCommand()` implementieren + `#include` |
| 5c | `src/uart/uart_wifi_config.cpp` | Dispatcher-Eintrag in `processCommand()` |

---

## Schnelltest nach Integration

```bash
# Bauen
pio run -e esp32-s3-devkitm-1

# Hochladen
pio run -t upload -e esp32-s3-devkitm-1

# UART-Test
hcsr04 measure

# REST-Test
curl http://<IP>/hcsr04/measure
```


Diese Anleitung beschreibt den vollständigen Weg — vom Treiber bis zur REST-API und UART-Schnittstelle — am Beispiel eines fiktiven Distanzsensors `hcsr04`.

---

## Projektstruktur (Übersicht)

```
lib/                        ← Hardware-Treiber (je ein Unterordner pro Gerät)
│   motor_28byj48/          ← Beispiel: Schrittmotortreiber
│   led_neopixel/
│   button/
│   servo/
│   ...
│
src/                        ← Applikations- & Systemcode
│   main.cpp
│   hal/                    ← Hardware Abstraction Layer (Pin-Konfiguration)
│   wifi/                   ← WiFi-Manager
│   api/                    ← REST-API (Routen & Handler)
│   uart/                   ← UART-Kommandointerface
│   network/                ← Netzwerk-Client
│
include/                    ← System-Header (passend zu src/)
│   hal/
│   wifi/
│   api/
│   uart/
│   network/
│
Doc/                        ← Dokumentation
diagrams/                   ← PlantUML-Diagramme
```

**Faustregel:**
- Alles, was direkt mit einem Stück Hardware spricht → `lib/<treiber>/`
- Alles, was das System zusammenhält (WiFi, HTTP, UART) → `src/<subsystem>/`

---

## Schritt 1 – Treiber anlegen (`lib/`)

Lege einen neuen Ordner unter `lib/` an. Name = Gerät/Chip-Bezeichnung in Kleinbuchstaben.

```
lib/
└── hcsr04/
    ├── hcsr04.h
    └── hcsr04.cpp
```

### `lib/hcsr04/hcsr04.h`

```cpp
#ifndef HCSR04_H
#define HCSR04_H

#include <Arduino.h>

// Konfiguration
#define HCSR04_DEFAULT_TRIG_PIN  10
#define HCSR04_DEFAULT_ECHO_PIN  11

// Initialisierung
void setupHCSR04(int trigPin, int echoPin);

// Messung
float hcsr04MeasureCm();

// Status-Typ
typedef struct {
    float lastDistanceCm;
    bool  sensorReady;
} HCSR04Status;

HCSR04Status getHCSR04Status();

#endif // HCSR04_H
```

**Konventionen für Treiber-Header:**
- Guard-Makro: `#ifndef <NAME>_H`
- Nur `<Arduino.h>` und gerätespezifische Systemincludes
- Keine Abhängigkeiten zu anderen Projektmodulen
- Status-Struct für Statusabfrage (wird von API und UART genutzt)
- Funktionspräfix = Gerätename (`hcsr04...`)

### `lib/hcsr04/hcsr04.cpp`

```cpp
#include "hcsr04.h"

static int _trigPin = HCSR04_DEFAULT_TRIG_PIN;
static int _echoPin = HCSR04_DEFAULT_ECHO_PIN;
static HCSR04Status _status = { 0.0f, false };

void setupHCSR04(int trigPin, int echoPin) {
    _trigPin = trigPin;
    _echoPin = echoPin;
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    _status.sensorReady = true;
}

float hcsr04MeasureCm() {
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    long duration = pulseIn(_echoPin, HIGH, 30000UL);
    float dist = duration * 0.0343f / 2.0f;
    _status.lastDistanceCm = dist;
    return dist;
}

HCSR04Status getHCSR04Status() {
    return _status;
}
```

PlatformIO erkennt die neue Library automatisch — keine Eintragung in `platformio.ini` nötig.

---

## Schritt 2 – Pins registrieren (`include/hal/pin_config.h`)

Füge die neuen Pins zur `PinConfiguration`-Struct hinzu:

```cpp
// include/hal/pin_config.h  →  struct PinConfiguration
int hcsr04_trig_pin;   // NEU
int hcsr04_echo_pin;   // NEU
```

Trage Standardwerte in `src/hal/pin_config.cpp` → `initPinConfig()` ein:

```cpp
// src/hal/pin_config.cpp  →  initPinConfig()
currentPinConfig.hcsr04_trig_pin = HCSR04_DEFAULT_TRIG_PIN;
currentPinConfig.hcsr04_echo_pin = HCSR04_DEFAULT_ECHO_PIN;
```

---

## Schritt 3 – main.cpp verdrahten (`src/main.cpp`)

```cpp
// Treiber einbinden
#include "hcsr04.h"

void setup() {
    // ... andere setup()-Aufrufe ...
    setupHCSR04(currentPinConfig.hcsr04_trig_pin,
                currentPinConfig.hcsr04_echo_pin);
}
```

---

## Schritt 4 – REST-API Routen (`src/api/web_server.cpp`)

### 4a – Handler-Deklaration in `include/api/web_server.h`

```cpp
void handleHCSR04Status();   // NEU
void handleHCSR04Measure();  // NEU
```

### 4b – Handler implementieren in `src/api/web_server.cpp`

```cpp
#include "hcsr04.h"   // NEU oben hinzufügen

// Handler: GET /hcsr04/status
void handleHCSR04Status() {
    HCSR04Status s = getHCSR04Status();
    String json = "{";
    json += "\"ready\":" + String(s.sensorReady ? "true" : "false") + ",";
    json += "\"lastDistanceCm\":" + String(s.lastDistanceCm, 1);
    json += "}";
    server.send(200, "application/json", json);
}

// Handler: GET /hcsr04/measure
void handleHCSR04Measure() {
    float dist = hcsr04MeasureCm();
    String json = "{\"distanceCm\":" + String(dist, 1) + "}";
    server.send(200, "application/json", json);
}
```

### 4c – Routen registrieren in `setupWebServer()`

```cpp
void setupWebServer() {
    // ... bestehende Routen ...

    // HC-SR04
    server.on("/hcsr04/status",  HTTP_GET, handleHCSR04Status);
    server.on("/hcsr04/measure", HTTP_GET, handleHCSR04Measure);
}
```

**API-Konventionen:**
| Methode | Pfad-Muster | Zweck |
|---|---|---|
| GET | `/hcsr04/status` | Status / Konfiguration lesen |
| GET | `/hcsr04/measure` | Messung auslösen & Ergebnis |
| POST | `/hcsr04/config` | Parameter setzen |

---

## Schritt 5 – UART-Befehle (`src/uart/uart_wifi_config.cpp`)

### 5a – Hilfetexttabelle erweitern in `printHelp()`

```cpp
Serial.println("╠════════════════════════════════════════════════════════════╣");
Serial.println("║  hcsr04 measure          Misst Abstand in cm              ║");
Serial.println("║  hcsr04 status           Zeigt Sensor-Status              ║");
```

### 5b – Befehlsprozessor hinzufügen

```cpp
// Neue Funktion vor processCommand()
static void processHCSR04Command(const char* cmd) {

    if (strcasecmp(cmd, "measure") == 0) {
        float dist = hcsr04MeasureCm();
        Serial.printf("✓ Abstand: %.1f cm\n", dist);
        return;
    }

    if (strcasecmp(cmd, "status") == 0) {
        HCSR04Status s = getHCSR04Status();
        Serial.println("\n[HC-SR04 Status]");
        Serial.printf("  Bereit          : %s\n", s.sensorReady ? "Ja" : "Nein");
        Serial.printf("  Letzter Wert    : %.1f cm\n", s.lastDistanceCm);
        return;
    }

    Serial.printf("[Fehler] Unbekannter hcsr04-Befehl: 'hcsr04 %s'\n", cmd);
    printHelp();
}
```

### 5c – Dispatcher eintragen in `processCommand()`

```cpp
static void processCommand(const char* line) {
    // ... bestehende Dispatcher ...

    if (strncasecmp(line, "hcsr04 ", 7) == 0) {
        processHCSR04Command(line + 7);
        return;
    }
}
```

### 5d – Include hinzufügen (Dateianfang)

```cpp
#include "hcsr04.h"   // NEU
```

---

## Checkliste (Zusammenfassung)

| Schritt | Datei | Was tun |
|---|---|---|
| 1 | `lib/hcsr04/hcsr04.h` | Treiber-Header anlegen |
| 1 | `lib/hcsr04/hcsr04.cpp` | Treiber implementieren |
| 2 | `include/hal/pin_config.h` | Pins zur Struct hinzufügen |
| 2 | `src/hal/pin_config.cpp` | Standardwerte in `initPinConfig()` |
| 3 | `src/main.cpp` | `#include` + `setup<Name>()` in `setup()` |
| 4a | `include/api/web_server.h` | Handler deklarieren |
| 4b | `src/api/web_server.cpp` | Handler implementieren + `#include` |
| 4c | `src/api/web_server.cpp` | Route in `setupWebServer()` eintragen |
| 5a | `src/uart/uart_wifi_config.cpp` | Hilfetexttabelle erweitern |
| 5b | `src/uart/uart_wifi_config.cpp` | `processXxxCommand()` implementieren |
| 5c | `src/uart/uart_wifi_config.cpp` | Dispatcher-Eintrag in `processCommand()` |
| 5d | `src/uart/uart_wifi_config.cpp` | `#include "hcsr04.h"` |

---

## Schnelltest nach Integration

```bash
# Bauen
pio run -e esp32-s3-devkitm-1

# Hochladen
pio run -t upload -e esp32-s3-devkitm-1

# UART-Test
hcsr04 measure

# REST-Test
curl http://<IP>/hcsr04/measure
```
