# How to Add a New Hardware Module

This guide covers the full path — from driver to REST API and UART interface — using a fictional distance sensor `hcsr04` as an example.

---

## Project Structure (Overview)

```
src/                        <- Full project source code
|   main.cpp
|   drivers/                <- Hardware drivers (.cpp)
|   |   motor_28byj48.cpp
|   |   led_control.cpp
|   |   button_control.cpp
|   |   servo_control.cpp
|   |   ...
|   hal/                    <- Hardware Abstraction Layer (pin configuration)
|   |   pin_config.cpp
|   wifi/                   <- Wi-Fi manager
|   |   wifi_manager.cpp
|   api/                    <- REST API (routes & handlers)
|   |   web_server.cpp
|   uart/                   <- UART command interface
|   |   uart_wifi_config.cpp
|
include/                    <- All header files (flat)
|   pin_config.h
|   motor_28byj48.h
|   led_control.h
|   wifi_manager.h
|   web_server.h
|   uart_wifi_config.h
|   ...
|
Doc/                        <- Documentation
diagrams/                   <- PlantUML architecture diagrams
```

**Rules of thumb:**
- Hardware drivers (`.cpp`) -> `src/drivers/`
- Driver headers (`.h`) -> `include/` (flat, no subdirectories)
- System code (Wi-Fi, HTTP, UART, HAL) -> `src/<subsystem>/`
- Headers in `include/` are kept flat intentionally — PlatformIO's LDF only scans the root level of `include/`

---

## Step 1 — Create the Driver

Create the driver files. Use the device/chip name in lowercase.

```
src/drivers/hcsr04.cpp
include/hcsr04.h
```

### `include/hcsr04.h`

```cpp
#ifndef HCSR04_H
#define HCSR04_H

#include <Arduino.h>

// Configuration
#define HCSR04_DEFAULT_TRIG_PIN  10
#define HCSR04_DEFAULT_ECHO_PIN  11

// Initialization
void setupHCSR04(int trigPin, int echoPin);

// Measurement
float hcsr04MeasureCm();

// Status type
typedef struct {
    float lastDistanceCm;
    bool  sensorReady;
} HCSR04Status;

HCSR04Status getHCSR04Status();

#endif // HCSR04_H
```

**Driver header conventions:**
- Include guard: `#ifndef <NAME>_H`
- Only `<Arduino.h>` and device-specific system includes
- No dependencies on other project modules
- Status struct for state queries (used by both API and UART)
- Function prefix = device name (`hcsr04...`)

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

PlatformIO compiles all `.cpp` files in `src/` automatically — no entry in `platformio.ini` required.

---

## Step 2 — Register Pins (`include/pin_config.h`)

Add the new pins to the `PinConfiguration` struct:

```cpp
// include/pin_config.h  ->  struct PinConfiguration
int hcsr04_trig_pin;   // NEW
int hcsr04_echo_pin;   // NEW
```

Add default values in `src/hal/pin_config.cpp` -> `initPinConfig()`:

```cpp
// src/hal/pin_config.cpp  ->  initPinConfig()
currentPinConfig.hcsr04_trig_pin = HCSR04_DEFAULT_TRIG_PIN;
currentPinConfig.hcsr04_echo_pin = HCSR04_DEFAULT_ECHO_PIN;
```

---

## Step 3 — Wire into main.cpp (`src/main.cpp`)

```cpp
// Include driver
#include "hcsr04.h"

void setup() {
    // ... other setup() calls ...
    setupHCSR04(currentPinConfig.hcsr04_trig_pin,
                currentPinConfig.hcsr04_echo_pin);
}
```

---

## Step 4 — REST API Routes (`src/api/web_server.cpp`)

### 4a — Declare handlers in `include/web_server.h`

```cpp
void handleHCSR04Status();   // NEW
void handleHCSR04Measure();  // NEW
```

### 4b — Implement handlers in `src/api/web_server.cpp`

```cpp
#include "hcsr04.h"   // add at top of file

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

### 4c — Register routes in `setupWebServer()`

```cpp
void setupWebServer() {
    // ... existing routes ...

    // HC-SR04
    server.on("/hcsr04/status",  HTTP_GET, handleHCSR04Status);
    server.on("/hcsr04/measure", HTTP_GET, handleHCSR04Measure);
}
```

**API conventions:**

| Method | Path pattern | Purpose |
|--------|-------------|---------|
| GET | `/hcsr04/status` | Read status / configuration |
| GET | `/hcsr04/measure` | Trigger measurement and return result |
| POST | `/hcsr04/config` | Set parameters |

---

## Step 5 — UART Commands (`src/uart/uart_wifi_config.cpp`)

### 5a — Extend the help text in `printHelp()`

```cpp
Serial.println("  hcsr04 measure    Measure distance in cm");
Serial.println("  hcsr04 status     Show sensor status");
```

### 5b — Add command processor (before `processCommand()`)

```cpp
#include "hcsr04.h"   // add at top of file

static void processHCSR04Command(const char* cmd) {

    if (strcasecmp(cmd, "measure") == 0) {
        float dist = hcsr04MeasureCm();
        Serial.printf("OK Distance: %.1f cm\n", dist);
        return;
    }

    if (strcasecmp(cmd, "status") == 0) {
        HCSR04Status s = getHCSR04Status();
        Serial.println("\n[HC-SR04 Status]");
        Serial.printf("  Ready        : %s\n", s.sensorReady ? "Yes" : "No");
        Serial.printf("  Last value   : %.1f cm\n", s.lastDistanceCm);
        return;
    }

    Serial.printf("[Error] Unknown hcsr04 command: 'hcsr04 %s'\n", cmd);
    printHelp();
}
```

### 5c — Add dispatcher entry in `processCommand()`

```cpp
static void processCommand(const char* line) {
    // ... existing dispatchers ...

    if (strncasecmp(line, "hcsr04 ", 7) == 0) {
        processHCSR04Command(line + 7);
        return;
    }
}
```

---

## Checklist (Summary)

| Step | File | Action |
|------|------|--------|
| 1 | `include/hcsr04.h` | Create driver header |
| 1 | `src/drivers/hcsr04.cpp` | Implement driver |
| 2 | `include/pin_config.h` | Add pins to struct |
| 2 | `src/hal/pin_config.cpp` | Add defaults in `initPinConfig()` |
| 3 | `src/main.cpp` | `#include` + `setup<Name>()` in `setup()` |
| 4a | `include/web_server.h` | Declare handlers |
| 4b | `src/api/web_server.cpp` | Implement handlers + `#include` |
| 4c | `src/api/web_server.cpp` | Register route in `setupWebServer()` |
| 5a | `src/uart/uart_wifi_config.cpp` | Extend help text |
| 5b | `src/uart/uart_wifi_config.cpp` | Implement `processXxxCommand()` + `#include` |
| 5c | `src/uart/uart_wifi_config.cpp` | Add dispatcher entry in `processCommand()` |

---

## Quick Test After Integration

```bash
# Build
pio run -e esp32-s3-devkitm-1

# Upload
pio run -t upload -e esp32-s3-devkitm-1

# UART test
hcsr04 measure

# REST test
curl http://<IP>/hcsr04/measure
```