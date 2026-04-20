# PositionUnit API Commands

**Base URL:** `http://<ESP32_IP>` *(replace with your device's IP)*  
**mDNS:** `http://MotorModule.local`  
**Port:** 80

---

## RGB LED Control

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/color?index=<0-6>&ledId=<N>` | Preset color (0=Red, 1=Green, 2=Blue, 3=Yellow, 4=Purple, 5=Orange, 6=White) |
| GET | `/hexcolor?hex=<RRGGBB>&ledId=<N>` | Custom hex color (without #, e.g. `FF00FF`) |
| GET | `/setBrightness?value=<0-255>` | Set LED brightness |
| GET | `/setLedPin?ledId=<N>&pin=<P>&count=<C>` | Configure LED pin and pixel count (NVS) |

---

## Servo Control

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/setServo?angle=<0-180>` | Move servo to angle |
| GET | `/setServoPin?pin=<P>` | Set servo pin (NVS) |

---

## 28BYJ-48 Stepper Motor

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/motor28byj48?action=moveRelative&steps=<N>&direction=<1\|-1>&speed=<0-100>` | Move relative |
| GET | `/motor28byj48?action=moveToPosition&position=<N>&speed=<N>` | Move to absolute position |
| GET | `/motor28byj48?action=home` | Move to home position |
| GET | `/motor28byj48?action=calibrate` | Set position to 0 |
| GET | `/motor28byj48?action=getStatus` | Get status |
| GET | `/motor28byj48?action=setPins&pin1=<P>&pin2=<P>&pin3=<P>&pin4=<P>` | Set IN1–IN4 pins (NVS) |

---

## NEMA 17/23 Advanced Motor

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/advancedMotor?...` | Advanced motor control |
| GET | `/motorStatus` | Get motor status |
| GET | `/motorStop` | Stop motor |
| GET | `/motorHome` | Start homing sequence |
| GET | `/motorJog?...` | Jog movement |
| GET | `/motorCalibrate` | Calibrate |
| GET | `/setHomingMode?...` | Set homing mode |
| GET | `/passButton?...` | Button passes |

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
| GET | `/` | Web-Interface Hauptseite |
| GET | `/reboot` | ESP32 neu starten |
| GET | `/systemInfo` | System-Informationen abrufen |
| POST | `/saveWiFiConfig` | Save Wi-Fi configuration |