# PositionUnit API Commands

**Base URL:** `http://<ESP32_IP>` *(replace with your device's IP)*  
**mDNS:** `http://D-ActorNode.local`  
**Port:** 80

> **Multi-instance parameters** — wherever `ledId`, `servoId` or `motorId` appear, the parameter is **optional** and defaults to `1` if omitted.

---

## RGB LED Control

Up to **3 independent LED outputs** (ledId = 1, 2, 3).

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/color?index=<0-6>` | Preset color on LED 1 |
| GET | `/color?index=<0-6>&ledId=<1-3>` | Preset color on specific LED |
| GET | `/hexcolor?hex=<RRGGBB>` | Custom hex color on LED 1 |
| GET | `/hexcolor?hex=<RRGGBB>&ledId=<1-3>` | Custom hex color on specific LED |
| GET | `/setBrightness?value=<0-255>` | Set brightness (all outputs) |
| GET | `/setLedPin?pin=<P>&count=<C>` | Set pin + pixel count for LED 1 (NVS) |
| GET | `/setLedPin?ledId=<1-3>&pin=<P>&count=<C>` | Set pin + pixel count for specific LED (NVS) |

**Color index table:**

| Index | Color |
|-------|-------|
| 0 | Red |
| 1 | Green |
| 2 | Blue |
| 3 | Yellow |
| 4 | Purple |
| 5 | Orange |
| 6 | White |

**Examples:**
```
GET /color?index=1                    → LED 1 green
GET /color?index=1&ledId=2            → LED 2 green
GET /hexcolor?hex=FF0000&ledId=3      → LED 3 red
```

---

## Servo Control

Up to **3 independent servos** (servoId = 1, 2, 3).

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/setServo?angle=<0-180>` | Move servo 1 to angle |
| GET | `/setServo?angle=<0-180>&servoId=<1-3>` | Move specific servo to angle |
| GET | `/setServoPin?pin=<P>` | Set pin for servo 1 (NVS) |
| GET | `/setServoPin?pin=<P>&servoId=<1-3>` | Set pin for specific servo (NVS) |

**Examples:**
```
GET /setServo?angle=90                → Servo 1 to 90°
GET /setServo?angle=90&servoId=2      → Servo 2 to 90°
GET /setServo?angle=45&servoId=3      → Servo 3 to 45°
GET /setServoPin?pin=19&servoId=2     → Assign GPIO19 to Servo 2 (NVS)
```

---

## 28BYJ-48 Stepper Motor

Single instance — **no motorId parameter**.

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/motor28byj48?action=moveRelative&steps=<N>&direction=<1\|-1>&speed=<0-100>` | Move relative |
| GET | `/motor28byj48?action=moveToPosition&position=<N>&speed=<N>` | Move to absolute position |
| GET | `/motor28byj48?action=home` | Move to home position |
| GET | `/motor28byj48?action=calibrate` | Set current position to 0 |
| GET | `/motor28byj48?action=getStatus` | Get status |
| GET | `/motor28byj48?action=setPins&pin1=<P>&pin2=<P>&pin3=<P>&pin4=<P>` | Set IN1–IN4 pins (NVS) |

---

## NEMA 17/23 Advanced Motor

Up to **3 independent motors** (motorId = 1, 2, 3).

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/advancedMotor?action=moveTo&position=<N>&speed=<N>` | Move motor 1 to absolute position |
| GET | `/advancedMotor?action=moveTo&position=<N>&speed=<N>&motorId=<1-3>` | Move specific motor to absolute position |
| GET | `/advancedMotor?action=moveRelative&steps=<N>&speed=<N>` | Move motor 1 relative |
| GET | `/advancedMotor?action=moveRelative&steps=<N>&speed=<N>&motorId=<1-3>` | Move specific motor relative |
| GET | `/advancedMotor?action=setHome` | Set home for motor 1 |
| GET | `/advancedMotor?action=setHome&motorId=<1-3>` | Set home for specific motor |
| GET | `/motorStatus` | Get status of motor 1 (JSON) |
| GET | `/motorStatus?motorId=<1-3>` | Get status of specific motor (JSON) |
| GET | `/motorStop` | Stop motor 1 |
| GET | `/motorStop?motorId=<1-3>` | Stop specific motor |
| GET | `/motorHome` | Start homing for motor 1 |
| GET | `/motorHome?motorId=<1-3>` | Start homing for specific motor |
| GET | `/motorCalibrate` | Calibrate motor 1 |
| GET | `/setHomingMode?...` | Set homing mode |
| GET | `/passButton?...` | Button pass sequence |

**Examples:**
```
GET /advancedMotor?action=moveTo&position=200&speed=60
GET /advancedMotor?action=moveTo&position=200&speed=60&motorId=2
GET /motorStatus?motorId=3
GET /motorStop?motorId=2
```

---

## Button

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/getButtonState` | Get current button state |
| GET | `/setButtonEnabled?enabled=<0\|1>` | Enable / disable button |
| GET | `/setButtonPin?pin=<P>` | Set button pin (NVS) |

---

## Pin Configuration

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/getPinConfig` | Current pin configuration as JSON |
| GET | `/pinConfig` | Pin configuration web UI |
| GET | `/resetPinConfig` | Reset pin configuration to defaults |
| POST | `/savePinConfig` | Save pin configuration |

---

## Device Information (NVS)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/getDeviceInfo` | Get device information |
| GET | `/setDeviceInfo?deviceName=<N>&deviceNumber=<N>&configuration=<C>&description=<D>` | Set device information (NVS) |

---

## Device Registry (NVS)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/devices` | List all registry entries |
| GET | `/devices/info?id=<N>` | Get single entry |
| GET | `/devices/remove?id=<N>` | Remove entry |
| GET | `/devices/clear` | Clear registry completely |
| POST | `/devices/add` | Add device generically (JSON body with `type`) |
| POST | `/leds` | Add LED device |
| POST | `/buttons` | Add button device |
| POST | `/motors` | Add motor device |
| POST | `/servos` | Add servo device |
| POST | `/steppers` | Add stepper device |

**Example JSON body for POST /leds:**
```json
{
  "name": "LED1",
  "pin": 38,
  "count": 1
}
```

---

## System

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Web interface main page |
| GET | `/reboot` | Reboot ESP32 |
| GET | `/systemInfo` | System information |
| POST | `/saveWiFiConfig` | Save Wi-Fi configuration |