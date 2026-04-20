# PositionUnit_with_API - Feature and Architecture Report

## 1. Purpose

This firmware controls an ESP32-S3 position unit with a web UI and REST API.
It integrates multiple actuators and sensors into a unified control interface:

- NEMA 17/23 (advanced stepper), up to 3 motor instances
- 28BYJ-48 stepper
- Servo, up to 3 channels
- LED (NeoPixel), up to 3 outputs each with configurable LED count
- Button input

Key property: pin and system configuration are stored persistently (Preferences/NVS) so settings survive reboots.

---

## 2. Main Features

### 2.1 Multi-Instance Control

- `motorId` for NEMA 17/23 endpoints (1..3)
- `servoId` for servo endpoints (1..3)
- `ledId` for LED endpoints (1..3)

Multiple identical actuators can be independently configured and addressed.

### 2.2 Persistent Configuration

The following data is persisted:

- NEMA 17/23: `STEP/DIR/EN` per motor instance
- Servo: pin per channel
- LED: pin + count per output
- 28BYJ-48: 4 control pins
- Button pin
- Wi-Fi (SSID, password, hostname)
- Device info (name, number, configuration, description)

Data is stored in NVS and applied to modules on boot.

### 2.3 Web UI with Tabs

The interface is an embedded HTML/JS page in `src/api/web_server.cpp`.

Tabs:
- Motor Control
- 28BYJ-48 Motor
- Servo Control
- LED Control
- Info (Button)
- Status & Info

### 2.4 API-First Design

All UI actions call REST endpoints. This enables:

- Browser control
- API testing via Notebook/Postman
- External integrations

### 2.5 Pin UX

- Pin fields use dropdown selectors
- Unavailable pins are visually indicated
- Collapsible pinout images in pin-assignment sections
- Pinout image is zoomable (modal with mouse-wheel zoom)

### 2.6 Stability Improvements

- Guard against invalid pins on critical modules
- Improved setup order (servo initialization late in boot), so stored servo pins take effect after reboot
- Serial output consistently at 115200 baud

---

## 3. Software Architecture

### 3.1 File Structure (Core Modules)

| File | Purpose |
|------|---------|
| `src/main.cpp` | Bootstrap and main loop |
| `src/api/web_server.cpp` | UI + HTTP routing + API handlers |
| `src/hal/pin_config.cpp` | Persistence and configuration model (NVS) |
| `src/drivers/servo_control.cpp` | Multi-channel servo (LEDC) |
| `src/drivers/advanced_motor.cpp` | NEMA 17/23 control (multi-instance) |
| `src/drivers/led_control.cpp` | NeoPixel outputs (runtime-configurable) |
| `src/drivers/motor_28byj48.cpp` | 28BYJ-48 logic |
| `src/drivers/button_control.cpp` | Button handling with debounce |
| `src/drivers/device_registry.cpp` | NVS-backed device registry |
| `src/wifi/wifi_manager.cpp` | Wi-Fi connection and reconnect |
| `src/uart/uart_wifi_config.cpp` | UART command interface |

### 3.2 Boot and Runtime Flow

```text
Power On / Reset
   |
   v
main.cpp::setup()
   |
   +--> initPinConfig()
   |      +--> loadPinConfig() from NVS
   |      +--> sanitize/apply to global module pins
   |
   +--> initDeviceInfo()
   +--> initDeviceRegistry()
   +--> setupLEDs()
   +--> setupAdvancedMotor()
   +--> setup28BYJ48Motor()
   +--> setupButton()
   +--> setupServo()   (late, so stored servo pin wins)
   +--> setupWiFi()
   +--> setupWebServer()
   |
   v
main.cpp::loop()
   +--> handleWebServerRequests()
   +--> updateMotor()
   +--> getButtonState()
   +--> checkWiFiConnection()
   +--> handleUartWifiConfig()
```

### 3.3 API Request Flow

```text
Browser UI action
   |
   v
fetch('/savePinConfig', ...)
   |
   v
web_server.cpp::handleSavePinConfig()
   |
   +--> setXxxPinById(...)
   |      +--> pin_config.cpp::savePinConfig()  --> NVS write
   |
   +--> reconfigure/setup module at runtime
   |
   v
HTTP response to UI
```

### 3.4 Persistence Model

```text
NVS namespace: pin_config

  m28_p1..m28_p4
  nema1_stp/dir/en, nema2_..., nema3_...
  servo1_p, servo2_p, servo3_p
  led1_p, led2_p, led3_p
  led1_c, led2_c, led3_c
  btn_p
  wifi_ssid, wifi_pass, wifi_host
  valid

NVS namespace: device_info

  deviceName
  deviceNumber
  configuration
  description

NVS namespace: dev_registry

  e0..e31  (DeviceEntry JSON)
  nextid
```

### 3.5 Module Relationships

```text
                 +---------------------+
                 |     web_server      |
                 |  UI + REST API      |
                 +----------+----------+
                            |
        +-------------------+-------------------+
        |                   |                   |
        v                   v                   v
 +-------------+    +---------------+    +-------------+
 | pin_config  |    | servo_control |    | led_control |
 | NVS model   |    | LEDC channels |    | NeoPixel    |
 +------+------+    +-------+-------+    +------+------+
        |                   |                   |
        v                   v                   v
 +-------------+    +---------------+    +-------------+
 | advanced_   |    | motor_28byj48 |    | button_     |
 | motor       |    | step sequence |    | control     |
 +------+------+    +---------------+    +-------------+
        |
        v
 +------------------+      +------------------+
 | wifi_manager     |      | device_registry  |
 | STA / AP / retry |      | NVS-backed store |
 +------------------+      +------------------+
```

---

## 4. Key Endpoints (Summary)

Control:
- `GET /setServo?servoId=<1..3>&angle=<0..180>`
- `GET /advancedMotor?...&motorId=<1..3>`
- `GET /color?ledId=<1..3>&index=<0..6>`
- `GET /hexcolor?ledId=<1..3>&hex=<RRGGBB>`
- `GET /setBrightness?ledId=<1..3>&value=<0..255>`

Configuration:
- `GET /pinConfig`
- `POST /savePinConfig`
- `GET /resetPinConfig`
- `POST /saveWiFiConfig`

Device Registry:
- `GET /devices`
- `GET /devices/info?id=<N>`
- `GET /devices/remove?id=<N>`
- `GET /devices/clear`
- `POST /leds | /buttons | /motors | /servos | /steppers`

System:
- `GET /systemInfo`
- `GET /getDeviceInfo`
- `GET /setDeviceInfo?...`
- `GET /reboot`

---

## 5. Implementation Status

| Feature | Status |
|---------|--------|
| Multi-servo | Done |
| Multi-LED with count per output | Done |
| Multi-NEMA 17/23 | Done |
| Pinout-based UI helpers | Done |
| Zoomable pinout image | Done |
| EEPROM/NVS persistence for pin and device data | Done |
| Boot stability and initialization order | Done |
| Device registry with NVS persistence | Done |
| REST shortcuts (POST /leds, /buttons, ...) | Done |
| GET /devices/clear + UART registry clear | Done |
| GET /reboot | Done |

---

## 6. Build and Operation

### 6.1 Build / Upload

Environment: `esp32-s3-devkitm-1`

```text
platformio run --environment esp32-s3-devkitm-1
platformio run --target upload --environment esp32-s3-devkitm-1 --upload-port COM7
platformio device monitor --port COM7 --baud 115200 --filter direct
```

### 6.2 Usage Notes

- When changing pins, always verify the physical wiring.
- Save+Apply writes configuration persistently.
- After reboot, stored values are automatically loaded.

---

## 7. Summary

The firmware is structured as a modular, API-driven embedded controller.
Core strengths are multi-instance capability, persistent configuration and a comprehensive web UI.
The current architecture allows new components, endpoints and validation rules to be added without fundamentally changing the overall system logic.