# D-ActorNode — Projektbeschreibung

**Autor:** Marc Nauendorf  
**Datum:** 21. April 2026  
**Lizenz:** MIT  
**Repository:** [GitHub – D-ActorNode](https://github.com/Nr44suessauer/D-ActorNode)

---

## 1. Projektübersicht

**D-ActorNode** ist eine eingebettete Steuerungsfirmware für den **ESP32-S3**, die im Rahmen des übergeordneten **I-Scan**-Projekts entwickelt wurde. Die Firmware macht einen ESP32-S3 zu einem eigenständigen, netzwerkfähigen Aktor-Knoten, der verschiedene Hardware-Komponenten lokal und über WLAN ansteuern kann.

Das System ermöglicht die Steuerung von:
- **Schrittmotoren** (28BYJ-48 unipolar und NEMA 17/23 bipolar)
- **Servo-Motoren** (bis zu 3 Kanäle)
- **NeoPixel RGB-LED** (konfigurierbare Anzahl und Pin)
- **Taster** mit ISR-basierter Entprellung

Die Steuerung erfolgt wahlweise über:
- **REST-API** (HTTP auf Port 80)
- **Integriertes Web-Interface** (im Flash des ESP32 eingebettet)
- **UART-Kommandoschnittstelle** (seriell, 115200 Baud)

Alle Konfigurationen werden persistent im **NVS (Non-Volatile Storage)** des ESP32 gespeichert und überleben Neustarts.

---

## 2. Hardware-Plattform

| Eigenschaft         | Wert                          |
|---------------------|-------------------------------|
| Mikrocontroller     | ESP32-S3                      |
| Board               | `esp32-s3-devkitm-1`          |
| Framework           | Arduino (via PlatformIO)      |
| Upload-Port         | COM7                          |
| Upload-Baudrate     | 921 600                       |
| Monitor-Baudrate    | 115 200                       |
| Bibliotheken        | Adafruit NeoPixel, ArduinoJson |

---

## 3. Softwarearchitektur

### 3.1 Modulstruktur

Die Firmware ist konsequent in Module aufgeteilt. Jedes Modul besteht aus einer Header-Datei (`include/`) und einer Implementierungsdatei (`src/drivers/`, `src/hal/`, `src/uart/`, `src/wifi/`, `src/api/`).

```
src/
├── main.cpp                  ← Einstiegspunkt (setup / loop)
├── api/
│   └── web_server.cpp        ← HTTP-Server, REST-Handler, eingebettetes HTML/JS-UI
├── drivers/
│   ├── advanced_motor.cpp    ← NEMA 17/23 Schrittmotor (nicht-blockierend)
│   ├── button_control.cpp    ← GPIO-Taster mit ISR + Entprellung
│   ├── device_registry.cpp   ← NVS-gestützte Geräteliste
│   ├── led_control.cpp       ← NeoPixel RGB-LED
│   ├── motor_28byj48.cpp     ← 28BYJ-48 Schrittsequenzer
│   └── servo_control.cpp     ← LEDC-basierte Servo-Steuerung
├── hal/
│   └── pin_config.cpp        ← NVS-Modell für alle Pin-Zuordnungen
├── uart/
│   └── uart_wifi_config.cpp  ← Serieller Kommando-Dispatcher
└── wifi/
    └── wifi_manager.cpp      ← WLAN (STA / AP / Reconnect)
```

### 3.2 Boot-Reihenfolge (`setup()`)

Die Initialisierungsreihenfolge in `main.cpp` ist bewusst festgelegt, da spätere Module auf früher initialisierte NVS-Werte zugreifen:

1. `initPinConfig()` — lädt alle Pin-Zuordnungen aus dem NVS
2. `initDeviceInfo()` — lädt Gerätebezeichnung, -nummer und Beschreibung
3. `initDeviceRegistry()` — lädt gespeicherte dynamische Geräte
4. `setupLEDs()` — initialisiert NeoPixel
5. `setupAdvancedMotor()` — NEMA-Treiber initialisieren
6. `setup28BYJ48Motor()` — 28BYJ-48 Treiber initialisieren
7. `setupButton()` — Taster und Interrupt registrieren
8. `setupServo()` — LEDC-Kanäle und Servo-Pins setzen
9. `setupWiFi()` — WLAN verbinden oder AP starten
10. `setupWebServer()` — HTTP-Routen registrieren, Server starten
11. `setupUartWifiConfig()` — UART-Puffer initialisieren

### 3.3 Haupt-Loop (`loop()`)

```cpp
handleWebServerRequests();   // HTTP-Anfragen verarbeiten
handleUartWifiConfig();      // Serielle Kommandos einlesen
updateAdvancedMotor();       // Nicht-blockierende Motorschritte ausführen
```

---

## 4. Kommunikationsschnittstellen

### 4.1 REST-API (HTTP Port 80)

Der integrierte `WebServer` (ESP32 Arduino-Bibliothek) stellt alle Endpunkte auf Port 80 bereit. Vollständige Dokumentation in `API-Calls.md`.

**LED-Steuerung** *(bis zu 3 Instanzen, `ledId` optional, Standard = 1)*

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/color?index=<0-6>` | Farbe per Index auf LED 1 setzen |
| GET | `/color?index=<0-6>&ledId=<1-3>` | Farbe per Index auf bestimmter LED setzen |
| GET | `/hexcolor?hex=<RRGGBB>` | Hex-Farbe auf LED 1 setzen |
| GET | `/hexcolor?hex=<RRGGBB>&ledId=<1-3>` | Hex-Farbe auf bestimmter LED setzen |
| GET | `/setBrightness?value=<0-255>` | Helligkeit setzen (alle Ausgänge) |
| GET | `/setLedPin?pin=<P>&count=<C>` | LED 1 Pin und Pixelanzahl (NVS) |
| GET | `/setLedPin?ledId=<1-3>&pin=<P>&count=<C>` | LED-Pin und Pixelanzahl für bestimmte LED (NVS) |

**Servo** *(bis zu 3 Instanzen, `servoId` optional, Standard = 1)*

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/setServo?angle=<0-180>` | Servo 1 auf Winkel fahren |
| GET | `/setServo?angle=<0-180>&servoId=<1-3>` | Bestimmten Servo auf Winkel fahren |
| GET | `/setServoPin?pin=<P>` | Pin für Servo 1 setzen (NVS) |
| GET | `/setServoPin?pin=<P>&servoId=<1-3>` | Pin für bestimmten Servo setzen (NVS) |

**28BYJ-48 Schrittmotor**

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/motor28byj48?action=moveRelative&steps=<N>&direction=<±1>&speed=<0-100>` | Relativ fahren |
| GET | `/motor28byj48?action=moveToPosition&position=<N>&speed=<N>` | Absolut fahren |
| GET | `/motor28byj48?action=home` | Referenzpunkt anfahren |
| GET | `/motor28byj48?action=calibrate` | Position auf 0 setzen |
| GET | `/motor28byj48?action=getStatus` | Status abfragen |

**NEMA 17/23 Schrittmotor** *(bis zu 3 Instanzen, `motorId` optional, Standard = 1)*

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/advancedMotor?action=moveTo&position=<N>&speed=<N>` | Motor 1 absolut positionieren |
| GET | `/advancedMotor?action=moveTo&position=<N>&speed=<N>&motorId=<1-3>` | Bestimmten Motor absolut positionieren |
| GET | `/advancedMotor?action=moveRelative&steps=<N>&speed=<N>` | Motor 1 relativ fahren |
| GET | `/advancedMotor?action=moveRelative&steps=<N>&speed=<N>&motorId=<1-3>` | Bestimmten Motor relativ fahren |
| GET | `/advancedMotor?action=setHome&motorId=<1-3>` | Referenzpunkt für Motor setzen |
| GET | `/motorStatus?motorId=<1-3>` | Status abfragen (JSON) |
| GET | `/motorStop?motorId=<1-3>` | Motor stoppen |
| GET | `/motorHome?motorId=<1-3>` | Referenzfahrt starten |

**Taster**

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/getButtonState` | Aktuellen Taster-Zustand abfragen |
| GET | `/setButtonEnabled?enabled=<0\|1>` | Taster aktivieren / deaktivieren |

**Gerätekonfiguration & Registry**

| Methode | Endpunkt | Beschreibung |
|---------|----------|--------------|
| GET | `/getPinConfig` | Pin-Konfiguration als JSON |
| POST | `/savePinConfig` | Pin-Konfiguration speichern |
| GET | `/resetPinConfig` | Auf Standardwerte zurücksetzen |
| GET | `/devices` | Alle Registry-Einträge abrufen |
| POST | `/devices/add` | Gerät hinzufügen (HTTP 201 → Neustart erforderlich) |
| GET | `/devices/remove?id=<N>` | Gerät entfernen |
| GET | `/devices/clear` | Alle Geräte löschen |
| GET | `/reboot` | ESP32 neu starten |

### 4.2 UART-Kommandoschnittstelle

Alle Befehle werden über die serielle Verbindung (115200 Baud, LF-Zeilenende) gesendet. Der Dispatcher in `uart_wifi_config.cpp` erkennt das erste Schlüsselwort und leitet an die zuständige Subfunktion weiter.

**WiFi-Befehle**

| Befehl | Wirkung |
|--------|---------|
| `wifi ssid <Name>` | SSID vormerken |
| `wifi pass <Passwort>` | Passwort vormerken |
| `wifi host <Hostname>` | mDNS-Hostname vormerken |
| `wifi save` | Gespeicherte Einstellungen anwenden und neu verbinden |
| `wifi reset` | SSID/Passwort löschen, Gerät startet als reiner AP |
| `wifi status` | Verbindungsstatus, IP, Signalstärke |

**Device-Befehle**

| Befehl | Wirkung |
|--------|---------|
| `device name <Wert>` | Gerätename setzen (= AP-SSID bei fehlendem WLAN) |
| `device status` | Name, Nummer und Konfiguration anzeigen |

**Registry-Befehle**

| Befehl | Wirkung |
|--------|---------|
| `registry list` | Alle Einträge auflisten |
| `registry add <Typ> <Name> ...` | Gerät anlegen (Neustart erforderlich) |
| `registry remove <ID>` | Eintrag löschen |
| `registry clear` | Alle Einträge löschen |

Erfolgreiche Antworten sind am `✓`-Präfix erkennbar.

---

## 5. Module im Detail

### 5.1 WiFi-Manager (`wifi_manager.cpp`)

Der WiFi-Manager unterstützt zwei Modi:

- **STA-Modus:** Verbindung zu einem vorhandenen WLAN. Verbindungsaufbau mit 15-Sekunden-Timeout. Bei Misserfolg wird automatisch in den AP-Modus gewechselt.
- **AP-Modus (Fallback):** Der ESP32 öffnet ein offenes WLAN mit dem Gerätenamen als SSID. Im AP-Modus wird alle **60 Sekunden** automatisch versucht, die gespeicherte WLAN-Verbindung erneut herzustellen.

LED-Statusanzeige:
- `setColorByIndex(0)` → Rot (Neuverbindung läuft)
- `setColorByIndex(1)` → Grün (STA verbunden)
- `setColorByIndex(2)` → Blau (AP-Modus aktiv)

Standard-Hostname: `"D-ActorNode"`

### 5.2 Taster-Steuerung (`button_control.cpp`)

Der Taster an GPIO 45 nutzt einen **Interrupt-basierten Ansatz** zur Entprellung:

1. Bei jeder Flanke (CHANGE) setzt die ISR `isr_triggered = true` und speichert den Zeitstempel (`isr_time_ms = millis()`).
2. `getButtonState()` wertet den Zustand erst dann aus, wenn seit der letzten ISR-Flanke mindestens **25 ms** vergangen sind.
3. Der Status (aktiviert/deaktiviert) wird im NVS unter dem Namespace `"button_cfg"` gespeichert.

Zustandsmaschine:
- **IDLE:** `isr_triggered = false`, gibt `(buttonState == LOW)` zurück
- **DEBOUNCING:** `isr_triggered = true`, gibt alten `buttonState` zurück bis Debounce abgelaufen

### 5.3 Geräteregistry (`device_registry.cpp`)

Die Registry ist eine NVS-gestützte Tabelle mit bis zu **32 Einträgen**. Jeder Eintrag besteht aus:
- **ID** (auto-increment, ab 1)
- **Typ:** `DEV_BUTTON`, `DEV_LED`, `DEV_MOTOR` (28BYJ-48), `DEV_SERVO`, `DEV_STEPPER` (NEMA)
- **Name** (max. 32 Zeichen)
- **Config** (JSON-String, max. 256 Zeichen, typspezifische Parameter)

NVS-Namespace: `"dev_registry"`. Einträge werden unter den Schlüsseln `e0`, `e1`, … `eN` gespeichert.

Neue Geräte sind erst nach einem **Neustart** hardwareseitig aktiv, da `bootInitRegisteredDevices()` nur in `setup()` aufgerufen wird.

> **Hinweis:** Die Registry ist im aktuellen Betrieb standardmäßig **leer** (`registry list` → `0 device(s)`). Die direkte Hardware-Steuerung über die REST-Endpunkte (`/setServo`, `/motor28byj48`, usw.) und die Pin-Konfiguration im NVS-Namespace `pin_config` funktioniert vollständig **ohne** Registry-Einträge. Die Registry ist eine vorbereitete Infrastruktur für ein zukünftiges Discovery-Protokoll, bei dem ein übergeordnetes System alle Knoten per `GET /devices` befragen kann, um deren Bestückung automatisch zu ermitteln.

### 5.4 Pin-Konfiguration (`pin_config.cpp`)

Alle Hardware-Pin-Zuordnungen werden in der Struktur `PinConfiguration` zusammengefasst und im NVS-Namespace `"pin_config"` gespeichert. Das ermöglicht eine vollständig **softwareseitige Umkonfiguration** ohne Kompilierung:

- 4 Pins für den 28BYJ-48 Motor
- Bis zu 3 × 3 Pins (Step/Dir/Enable) für NEMA-Motoren
- Bis zu 3 Servo-Pins
- Bis zu 3 LED-Ausgänge mit je Pin und Pixelanzahl
- 1 Taster-Pin
- WLAN: SSID, Passwort, Hostname (je NVS-Namespace `"pin_config"`)

Gerätebezeichnung, -nummer und Beschreibung werden im Namespace `"device_info"` gespeichert.

### 5.5 LED-Steuerung (`led_control.cpp`)

NeoPixel-Steuerung über die Adafruit-Bibliothek. Farbpalette:

| Index | Farbe   | RGB         |
|-------|---------|-------------|
| 0     | Rot     | 255, 0, 0   |
| 1     | Grün    | 0, 255, 0   |
| 2     | Blau    | 0, 0, 255   |
| 3     | Gelb    | 255, 255, 0 |
| 4     | Lila    | 128, 0, 128 |
| 5     | Orange  | 255, 165, 0 |
| 6     | Weiß    | 255, 255, 255 |

### 5.6 NEMA-Schrittmotor (`advanced_motor.cpp`)

Bis zu **3 unabhängige Motoren**. Standardwerte: 200 Schritte/Umdrehung (1,8°/Schritt), maximal 500 RPM, Beschleunigung über 50 Schritte. Nicht-blockierende Ausführung über `updateAdvancedMotor()` im Loop. Unterstützt Homing, Jog-Modus und konfigurierbare Passier-Funktion für den Taster.

---

## 6. Persistenz-Modell (NVS)

| Namespace       | Inhalt                                      |
|-----------------|---------------------------------------------|
| `pin_config`    | Alle Hardware-Pin-Zuordnungen, WLAN-Daten   |
| `device_info`   | Gerätename, -nummer, Konfiguration, Beschreibung |
| `button_cfg`    | `enabled` (bool) — Taster aktiviert?        |
| `dev_registry`  | `e0`…`eN` (JSON), `nextid` (uint32)         |

---

## 7. PlantUML-Architekturdokumentation

Im Ordner `diagrams/` befinden sich 8 PlantUML-Diagramme, die die Architektur vollständig beschreiben:

| Datei | Typ | Inhalt |
|-------|-----|--------|
| `01_component.puml` | Komponenten | Abhängigkeiten zwischen allen Modulen |
| `02_classes.puml` | Klassen/Module | Interfaces und Beziehungen |
| `03_seq_boot.puml` | Sequenz | Boot-Ablauf (`setup()`) |
| `04_seq_uart.puml` | Sequenz | UART-Kommandoverarbeitung (4 Beispiele) |
| `05_seq_http.puml` | Sequenz | HTTP-Request-Handling (3 Beispiele) |
| `06_seq_button_isr.puml` | Sequenz | Button-ISR und Entprelllogik |
| `07_state_wifi.puml` | Zustandsautomat | WiFi-Manager-Zustände |
| `08_state_button.puml` | Zustandsautomat | Taster-Zustände (ENABLED/DISABLED, IDLE/DEBOUNCING) |

Die Diagramme wurden am **21. April 2026** gegen die Codebase geprüft und aktualisiert (Commit `6eafce4`).

---

## 8. Entwicklungsumgebung

- **IDE:** Visual Studio Code mit PlatformIO-Extension
- **Build-System:** PlatformIO (`pio run`, `pio upload`)
- **Upload:** UART-Bootloader, COM7, 921 600 Baud
- **Monitor:** `pio device monitor` (115 200 Baud, Echo aktiviert)
- **API-Tests:** Postman-Collection enthalten (`REST API basics- CRUD, test & variable.postman_collection.json`)
- **Notebook:** `ESP32_API_Test.ipynb` für automatisierte API-Tests

---

## 9. Instanzlimits und Erweiterbarkeit

### 9.1 Hardcodierte Maximalanzahlen

Jede Hardware-Klasse hat ein **compile-time Limit**, das in einer einzigen `#define`-Zeile im jeweiligen Header steckt. Darunter liegen statisch allokierte Arrays oder fest deklarierte Objekte.

| Modul | Konstante | Datei | Limit |
|---|---|---|---|
| Servo | `MAX_SERVOS` | `include/servo_control.h` | 3 |
| NeoPixel LED | `MAX_LED_OUTPUTS` | `include/led_control.h` | 3 |
| NEMA 17/23 Schrittmotor | `MAX_ADVANCED_MOTORS` | `include/advanced_motor.h` | 3 |
| 28BYJ-48 Schrittmotor | *(kein Array)* | `src/drivers/motor_28byj48.cpp` | 1 |

**Kann ich Instanz 4 über die Registry anlegen und ansteuern?**

Nein. Die Registry speichert den Eintrag im NVS, aber `bootInitRegisteredDevices()` kann kein viertes Objekt erzeugen, weil es nicht existiert. Das Verhalten je Typ:

- **Servo 4:** `isValidServoId(4)` → `false` — jede Ansteuerung bricht sofort ab. `channelForServo(4)` fällt auf `LEDC_CHANNEL_0` zurück (Servo 1).
- **LED 4:** `isValidLedId(4)` → `false` — kein `Adafruit_NeoPixel`-Objekt wird erzeugt. Registry-Eintrag bleibt passiv; `setupLEDs()` iteriert nur bis `MAX_LED_OUTPUTS`.
- **NEMA Motor 4:** Kein viertes `AdvancedStepperMotor`-Objekt deklariert — Zugriff nicht möglich.
- **28BYJ-48 Motor 2:** `bootInitRegisteredDevices()` ruft `setup28BYJ48MotorWithPins()` auf, was aber nur die globalen Pins der **einen** existierenden Instanz überschreibt.

### 9.2 Pins zur Laufzeit umbelegen

Was zur Laufzeit ohne Firmware-Update funktioniert, ist das **Umbelegen der Pins** bestehender Instanzen — alle Zuordnungen liegen im NVS:

| Modul | HTTP-Endpunkt / Funktion | Neustart nötig? |
|---|---|---|
| Servo 1–3 Pin | `GET /setServoPin?pin=<P>` | Nein |
| LED 1–3 Pin + Pixelanzahl | `GET /setLedPin?ledId=<N>&pin=<P>&count=<C>` | Nein |
| NEMA Motor 1–3 Pins | `reconfigurePins()` intern via `/advancedMotor` | Nein |
| 28BYJ-48 Pins | `GET /motor28byj48?action=setPins&pin1=…` | Nein |

### 9.3 Limit erhöhen (Codeänderung)

Um z. B. Servo 4 zu ermöglichen, sind genau **drei Stellen** zu ändern:

```c
// include/servo_control.h
#define MAX_SERVOS 4          // war: 3

// src/drivers/servo_control.cpp
int SERVO_GPIO_PINS[MAX_SERVOS] = {14, 15, 16, 17};   // Pin für Servo 4 ergänzen

static ledc_channel_t channelForServo(uint8_t servoId) {
    case 4: return LEDC_CHANNEL_3;                     // neuen case ergänzen
}
```

Für LED und NEMA-Motor gilt dasselbe Muster: `#define` erhöhen + Array-Initializer erweitern.

### 9.4 Allgemeine Erweiterbarkeit

1. **Neue Hardware-Module** werden nach dem Schema in `Doc/HOW_TO_ADD_MODULE.md` hinzugefügt: Header in `include/`, Implementierung in `src/drivers/`, Initialisierung in `main.cpp`, HTTP-Route in `web_server.cpp`, optional UART-Subcommand in `uart_wifi_config.cpp`.
2. **Dynamische Geräte** können zur Laufzeit über die Registry-API registriert werden, ohne Firmware-Update — aktivieren sich aber erst nach Neustart.
3. **Pin-Rekonfiguration** ist vollständig zur Laufzeit möglich — alle Zuordnungen werden im NVS gehalten.

---

## 10. Bekannte Besonderheiten

- `setupServo()` wird bewusst **als letztes** vor WiFi initialisiert, damit persistent gespeicherte Servo-Pins etwaige GPIO-Belegungen anderer Module überschreiben können.
- Der `WebServer` läuft auf Port 80; es gibt keinen HTTPS-Support (embedded target ohne ausreichenden Speicher für TLS).
- Die `bootInitRegisteredDevices()`-Funktion unterstützt **kein** Runtime-Re-Init für LED-Einträge aus der Registry — diese werden erst beim nächsten Reboot durch `setupLEDs()` aktiviert.
