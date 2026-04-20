# UART WiFi & Device Configuration

This document describes how to configure the Wi-Fi settings and device name of the ESP32 I-Scan controller via the serial console (UART). All settings are stored persistently in flash (NVS/Preferences) and survive reboots.

---

## Connection Settings

| Parameter    | Value    |
|--------------|----------|
| Baud Rate    | 115200   |
| Data Bits    | 8        |
| Parity       | None     |
| Stop Bits    | 1        |
| Line Ending  | LF or CRLF |

**Recommended tools:**
- PlatformIO Serial Monitor (`pio device monitor`)
- Arduino IDE Serial Monitor
- PuTTY / Tera Term (Windows)
- `screen /dev/ttyUSB0 115200` (Linux/macOS)

---

## Connection Behavior (Boot Sequence)

In AP mode the device automatically attempts to reconnect to the stored Wi-Fi network every **60 seconds**. On success it switches back to STA mode.

The **web interface** is reachable in both modes:
- **STA mode:** `http://<assigned-ip>` (see `wifi status`)
- **AP mode:** `http://192.168.4.1`

---

## Command Reference

### WiFi Commands

| Command                     | Description                                          |
|-----------------------------|------------------------------------------------------|
| `wifi help`                 | Show command overview                                |
| `wifi status`               | Show mode, SSID, IP, signal strength and AP info     |
| `wifi ssid <network name>`  | Set the Wi-Fi network name (SSID)                    |
| `wifi pass <password>`      | Set the Wi-Fi password                               |
| `wifi host <hostname>`      | Set the mDNS hostname                                |
| `wifi save`                 | Save all changes and reconnect                       |
| `wifi reset`                | Clear SSID/password, device starts as pure AP        |

### Device Commands

| Command                 | Description                                                          |
|-------------------------|----------------------------------------------------------------------|
| `device name <value>`   | Set device name (= AP SSID when no Wi-Fi). Applied immediately in AP mode. |
| `device status`         | Show current device name, number and configuration                   |

### Registry Commands

| Command                          | Description                           |
|----------------------------------|---------------------------------------|
| `registry list`                  | List all registry entries             |
| `registry add <type> <name> ...` | Add a device entry                    |
| `registry remove <id>`           | Remove an entry by ID                 |
| `registry clear`                 | Delete all registry entries           |

---

## Detailed Description

### `wifi status`

STA mode output:
```
[WiFi Status]
  Stored SSID  : OfficeNetwork
  mDNS hostname: MotorModule
  Device name  : MotorModule
  Mode         : Station (STA)
  Connected to : OfficeNetwork
  IP address   : 192.168.x.x
  Signal       : -61 dBm
```

AP mode output:
```
[WiFi Status]
  Stored SSID  : (not set)
  mDNS hostname: MotorModule
  Device name  : MotorModule
  Mode         : ACCESS POINT (no Wi-Fi available)
  AP IP        : 192.168.4.1
  Connected clients: 1
```

---

### `wifi ssid <network name>`

Stages the new SSID. Does not take effect until `wifi save`.

```
wifi ssid OfficeNetwork
OK SSID staged: "OfficeNetwork"  ->  run 'wifi save' to apply
```

Maximum length: 63 characters.

---

### `wifi pass <password>`

Stages the new password. Does not take effect until `wifi save`.

```
wifi pass MySecurePassword123
OK Password staged  ->  run 'wifi save' to apply
```

The password is not echoed to the console. Maximum length: 63 characters.

---

### `wifi host <hostname>`

Sets the mDNS hostname. The device becomes reachable under `<hostname>.local`. Independent from the device name.

```
wifi host iscan-lab-1
OK Hostname staged: "iscan-lab-1"  ->  run 'wifi save' to apply
```

Maximum length: 31 characters.

---

### `wifi save`

Writes all staged Wi-Fi changes to flash and reconnects.

```
wifi save
Wi-Fi config saved (SSID: OfficeNetwork, Hostname: MotorModule)
Reconnecting...
[WiFi] Connecting to: OfficeNetwork
[WiFi] Connected to: OfficeNetwork
[WiFi] Local IP:     192.168.x.x
```

---

### `wifi reset`

Clears SSID and password. Device starts as a pure Access Point.

```
wifi reset
Wi-Fi config reset (no network, AP mode active).
[WiFi] Starting Access Point: "MotorModule" (no password)
[WiFi] AP started - IP: 192.168.4.1
```

---

### `device name <value>`

Sets the device name. Used as AP SSID when no external Wi-Fi is available. If currently in AP mode, the name is updated immediately.

```
device name Unit-3
OK Device name set: "Unit-3"
```

Maximum length: 31 characters. Default: `MotorModule`.

---

### `device status`

```
[Device Status]
  Device name (AP SSID) : MotorModule
  Device number         : 0001
  Configuration         : Default Configuration
```

---

## Typical Workflows

### Initial setup at a new location

```
wifi status
wifi ssid OfficeNetwork
wifi pass SecretPassword
wifi host iscan-unit-1
wifi save
wifi status
```

### Rename device

```
device name Production-Unit-A
```

### Return to pure AP mode

```
wifi reset
```

---

## Example Session (Realistic Terminal Output)

The following shows a complete serial session – from connecting to the device through staging credentials, saving, and verifying the result.

```
--- Warte auf Verbindung ---
[System] Verbindung hergestellt – Baudrate 115200.
[System] Tippe "wifi status" oder "device status" zum Starten.

> wifi ssid HomeNetwork
✓ SSID staged: "HomeNetwork"  ->  use 'wifi save' to apply

> wifi pass S3cureP@ss!
✓ Password staged  ->  use 'wifi save' to apply

> wifi host iscan-unit-1
✓ Hostname staged: "iscan-unit-1"  ->  use 'wifi save' to apply

> device name IScan-Pos-01
✓ Device name set: "IScan-Pos-01"

> device number 001
✓ Device number set: "001"

> device config StandardScan
✓ Configuration set: "StandardScan"

> device desc "Positioniereinheit Schiene A"
✓ Description set: "Positioniereinheit Schiene A"

> device status

[Device Status]
  Device name (AP-SSID): IScan-Pos-01
  Device number        : 001
  Configuration        : StandardScan
  Description          : Positioniereinheit Schiene A

> wifi save
💾 Wi-Fi configuration saved (SSID: HomeNetwork, Hostname: iscan-unit-1)
   Reconnecting...
[WiFi] Hostname: iscan-unit-1
[WiFi] Connecting to Wi-Fi: HomeNetwork
..........
[WiFi] Connected to: HomeNetwork
[WiFi] Local IP:      192.168.1.42
[WiFi] Hostname:      iscan-unit-1

> wifi save
💾 Wi-Fi configuration saved (SSID: HomeNetwork, Hostname: iscan-unit-1)
   Reconnecting...
[WiFi] Hostname: iscan-unit-1
[WiFi] Connecting to Wi-Fi: HomeNetwork
.....
[WiFi] Connected to: HomeNetwork
[WiFi] Local IP:      192.168.1.42
[WiFi] Hostname:      iscan-unit-1

> wifi status

[Wi-Fi Status]
  Saved SSID    : HomeNetwork
  mDNS hostname : iscan-unit-1
  Device name/AP: IScan-Pos-01
  Mode          : Station (STA)
  Connected to  : HomeNetwork
  IP address    : 192.168.1.42
  Signal strength: -58 dBm

> wifi status

[Wi-Fi Status]
  Saved SSID    : HomeNetwork
  mDNS hostname : iscan-unit-1
  Device name/AP: IScan-Pos-01
  Mode          : Station (STA)
  Connected to  : HomeNetwork
  IP address    : 192.168.1.42
  Signal strength: -57 dBm
```

> **Note:** Calling `wifi save` a second time without new staged changes is safe – it re-applies the stored configuration and triggers a reconnect. The IP address may differ slightly between calls if the DHCP lease was refreshed.

---

## Notes

- Commands are case-insensitive. SSID, password and device name are case-sensitive.
- Both LF and CRLF line endings are accepted.
- If every STA connection attempt fails, the device automatically starts as an open Access Point.
- In AP mode the device checks every 60 seconds whether the configured Wi-Fi is reachable.
- LED indicator: Green = STA connected, Yellow = AP mode, Red = connection lost.

---

## Technical Details

| Component                | Details                                                            |
|--------------------------|--------------------------------------------------------------------|
| Storage (Wi-Fi)          | NVS namespace `pin_config` (Preferences)                           |
| Storage (device name)    | NVS namespace `device_info` (Preferences)                          |
| AP password              | None (open network)                                                |
| AP default IP            | 192.168.4.1                                                        |
| STA reconnect interval   | 60 seconds                                                         |
| Input buffer             | 256 bytes                                                          |
| Implementation           | `src/uart/uart_wifi_config.cpp`, `src/wifi/wifi_manager.cpp`       |
| Entry points             | `setupUartWifiConfig()` in `setup()`, `handleUartWifiConfig()` in `loop()` |