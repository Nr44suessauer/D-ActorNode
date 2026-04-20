# D-ActorNode

**Author:** Marc Nauendorf  
**License:** [MIT](LICENSE) — Free to use, modify, and distribute, including for commercial purposes.

## Project Overview

This project controls various hardware components (stepper motor, servo, RGB LED, button) using an ESP32-S3 controller. Control is possible both locally and via a network (Wi-Fi), including a web server and API interfaces. Development is done with PlatformIO.

---

## Folder and File Structure

```
D-ActorNode/
│
├── API-Calls.md                # Documentation of all available REST API endpoints
├── IMPLEMENTATION_REPORT.md    # Feature and architecture report
├── UART_WiFi_Config.md         # UART command reference (Wi-Fi / device config)
├── platformio.ini              # PlatformIO project configuration
├── ReadMe.md                   # This file
│
├── include/                    # Header files (flat, PlatformIO LDF requirement)
│   ├── advanced_motor.h        # NEMA 17/23 stepper – class + API
│   ├── button_control.h        # Button with ISR debounce
│   ├── device_registry.h       # NVS-backed device registry
│   ├── led_control.h           # NeoPixel LED control
│   ├── motor_28byj48.h         # 28BYJ-48 unipolar stepper
│   ├── pin_config.h            # Persistent pin + device configuration (NVS)
│   ├── servo_control.h         # Multi-channel servo (LEDC)
│   ├── uart_wifi_config.h      # UART command interface
│   ├── web_server.h            # HTTP server and REST API
│   └── wifi_manager.h          # Wi-Fi manager (STA / AP / retry)
│
├── lib/                        # Custom or external libraries
│   └── README
│
├── src/                        # Source code
│   ├── main.cpp                # Entry point (setup / loop)
│   ├── api/
│   │   └── web_server.cpp      # Embedded HTML UI + all HTTP handlers
│   ├── drivers/
│   │   ├── advanced_motor.cpp  # NEMA 17/23 non-blocking stepper
│   │   ├── button_control.cpp  # ISR + debounce
│   │   ├── device_registry.cpp # NVS device registry
│   │   ├── led_control.cpp     # NeoPixel (runtime-configurable)
│   │   ├── motor_28byj48.cpp   # 28BYJ-48 step sequencer
│   │   └── servo_control.cpp   # LEDC-based servo
│   ├── hal/
│   │   └── pin_config.cpp      # NVS read/write + module apply
│   ├── uart/
│   │   └── uart_wifi_config.cpp# Serial command dispatcher
│   └── wifi/
│       └── wifi_manager.cpp    # Wi-Fi connect / AP fallback / reconnect
│
├── diagrams/                   # PlantUML architecture diagrams
│   ├── 01_component.puml
│   ├── 02_classes.puml
│   ├── 03_seq_boot.puml
│   ├── 04_seq_uart.puml
│   ├── 05_seq_http.puml
│   ├── 06_seq_button_isr.puml
│   ├── 07_state_wifi.puml
│   └── 08_state_button.puml
│
└── Doc/
    └── archive/                # Archived / superseded source files
```

---

## Key Files and Their Purpose

- **platformio.ini** — Board selection (`esp32-s3-devkitm-1`), framework (`arduino`), libraries, build options.

- **API-Calls.md** — Overview of all REST API endpoints grouped by function.

- **IMPLEMENTATION_REPORT.md** — Feature list, architecture description and module relationships.

- **UART_WiFi_Config.md** — Serial command reference for Wi-Fi and device configuration.

- **src/main.cpp** — Entry point: initializes all modules in the correct order, runs the main loop.

- **src/api/web_server.cpp** — Embedded HTML/JS web UI and all HTTP route handlers.

- **src/hal/pin_config.cpp** — Persistent NVS model for all pin assignments and device information.

- **src/drivers/advanced_motor.cpp** — Non-blocking NEMA 17/23 stepper driver (up to 3 instances).

- **src/drivers/motor_28byj48.cpp** — 28BYJ-48 unipolar stepper with configurable pins.

- **src/drivers/servo_control.cpp** — Multi-channel servo using ESP32 LEDC (up to 3 channels).

- **src/drivers/led_control.cpp** — NeoPixel RGB LED control (runtime-configurable per output).

- **src/drivers/button_control.cpp** — GPIO button with ISR-based debounce.

- **src/drivers/device_registry.cpp** — NVS-backed registry for dynamically registered devices.

- **src/wifi/wifi_manager.cpp** — Wi-Fi management: STA connect, AP fallback, auto-reconnect.

- **src/uart/uart_wifi_config.cpp** — Serial command dispatcher (wifi / device / led / motor / button / registry).

---

## Settings & Configuration

- **Board & Framework:** Defined in `platformio.ini` — `esp32-s3-devkitm-1`, Arduino framework.

- **Libraries:** `Adafruit NeoPixel ^1.12.3`, `ArduinoJson ^7.3.1` (bblanchon). Managed by PlatformIO.

- **Pin Assignment:** All pin defaults are in `include/pin_config.h`. Assignments are stored in NVS and can be changed at runtime via the web UI, REST API or UART commands.

- **Network:** Default device name / AP SSID: `MotorModule`. Configure Wi-Fi via UART (`wifi ssid` / `wifi pass` / `wifi save`) or the web UI Status tab. Default AP IP: `192.168.4.1`.

- **Serial:** 115200 baud, LF line endings.

- **API:**  
  The HTTP API is documented in `API-Calls.md`. It enables control of all components over the network.

---

## Development & Extension

- **New hardware components** can be integrated by creating new modules (e.g., `sensor_control.cpp/h`).
- **Web server assets** are placed in the `data/` folder and uploaded to the ESP32 file system using PlatformIO.

---

## Notes

- Changes to pin assignments or hardware should always be documented in the header files.
- The API documentation (`API-Calls.md`) is essential for external control and should be kept up to date.

## Overview of Main Functions by Module

| Module/File                | Functions                                                                                 |
|---------------------------|------------------------------------------------------------------------------------------|
| **main.cpp**              | setup(), loop()                                                                           |
| **led_control.h/cpp**     | setupLEDs(), updateLEDs(), setColorByIndex(), setColorRGB(), setColorHSV(), setBrightness()|
| **servo_control.h/cpp**   | setupServo(), setServoAngle(), getServoAngle()                                            |
| **motor.h/cpp**           | setupMotor(), setMotorPins(), moveMotor(), moveMotorToPosition(), moveMotorWithSpeed()    |
| **button_control.h/cpp**  | setupButton(), getButtonState()                                                           |
| **wifi_manager.h/cpp**    | setupWiFi(), checkWiFiConnection()                                                        |
| **web_server.h/cpp**      | setupWebServer(), handleWebServerRequests(), handleRoot(), handleColorChange(),           |
|                           | handleHexColorChange(), handleServoControl(), handleMotorControl(),                       |
|                           | handleGetButtonState(), handleBrightness(), handleNotFound()                              |
| **network_client.h/cpp**  | (Network communication functions, e.g., send/receive data)                                |

