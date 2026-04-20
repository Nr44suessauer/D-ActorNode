#include "uart_wifi_config.h"
#include "pin_config.h"
#include "wifi_manager.h"
#include "motor_28byj48.h"
#include "led_control.h"
#include "button_control.h"
#include "device_registry.h"
#include <WiFi.h>
#include <string.h>

// Input buffer for serial characters
static char inputBuffer[256];
static int  inputPos = 0;

// Pending changes (not yet saved)
static char pendingSsid[64]       = "";
static char pendingPass[64]       = "";
static char pendingHost[32]       = "";
static char pendingDeviceName[32] = "";

// ─── Internal helper functions ────────────────────────────────────────────

static void printHelp() {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║         UART Configuration – Command List                   ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  wifi status              Show current Wi-Fi status         ║");
    Serial.println("║  wifi ssid <value>        Set SSID (network name)           ║");
    Serial.println("║  wifi pass <value>        Set password                      ║");
    Serial.println("║  wifi host <value>        Set mDNS hostname                 ║");
    Serial.println("║  wifi save                Save & reconnect                  ║");
    Serial.println("║  wifi reset               Reset Wi-Fi config to default     ║");
    Serial.println("║  wifi ip                  Show current IP address           ║");
    Serial.println("║  wifi help                Show this help                    ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  device name <value>      Set device name (= AP-SSID)       ║");
    Serial.println("║  device number <value>    Set device number                 ║");
    Serial.println("║  device config <value>    Set configuration label           ║");
    Serial.println("║  device desc <value>      Set device description            ║");
    Serial.println("║  device status            Show all device information       ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  motor status             Show motor status                 ║");
    Serial.println("║  motor move <steps> <dir> Rotate motor (dir: 1=CW, 0=CCW) ║");
    Serial.println("║  motor degrees <deg> <dir> Rotate by degrees (1=CW,0=CCW) ║");
    Serial.println("║  motor stop               Stop motor                        ║");
    Serial.println("║  motor home               Move to home position             ║");
    Serial.println("║  motor calibrate          Calibrate motor                   ║");
    Serial.println("║  motor speed <value>      Set motor speed                   ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  led color <r> <g> <b>    Set LED color (0-255 each)        ║");
    Serial.println("║  led brightness <value>   Set brightness (0-255)            ║");
    Serial.println("║  led index <n>            Set color by index                ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  button status            Show button state                 ║");
    Serial.println("║  button enable            Enable button                     ║");
    Serial.println("║  button disable           Disable button                    ║");
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.println("║  registry list            Show all registered devices       ║");
    Serial.println("║  registry add <type> <name> <k=v ...>  Add device          ║");
    Serial.println("║  registry remove <id>     Remove device                     ║");
    Serial.println("║  registry clear           Delete ALL registry entries       ║");
    Serial.println("║    Typen: button led motor servo stepper                    ║");
    Serial.println("║    Beispiel: registry add button MyBtn pin=45 pullup=1      ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
}

static void printPendingChanges() {
    bool hasPending = pendingSsid[0] || pendingPass[0] || pendingHost[0] || pendingDeviceName[0];
    if (!hasPending) return;

    Serial.println("[Pending changes - not yet saved]");
    if (pendingSsid[0])       Serial.printf("  SSID        : %s\n", pendingSsid);
    if (pendingPass[0])       Serial.println("  Password    : (set)");
    if (pendingHost[0])       Serial.printf("  Hostname    : %s\n", pendingHost);
    if (pendingDeviceName[0]) Serial.printf("  Device name : %s\n", pendingDeviceName);
    Serial.println("  -> Use 'wifi save' or 'device name' to save.");
}

// ─── wifi commands ────────────────────────────────────────────────────────

static void processWifiCommand(const char* cmd) {

    // ── wifi status ────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "status") == 0) {
        char ssid[64], pass[64], host[32];
        getWiFiConfig(ssid, pass, host);
        DeviceInfo info = getDeviceInfo();

        Serial.println("\n[Wi-Fi Status]");
        Serial.printf("  Saved SSID    : %s\n", ssid[0] ? ssid : "(not set)");
        Serial.printf("  mDNS hostname : %s\n", host);
        Serial.printf("  Device name/AP: %s\n", info.deviceName.c_str());

        if (wifi_ap_mode_active) {
            Serial.println("  Mode          : ACCESS POINT (no Wi-Fi reachable)");
            Serial.printf("  AP IP         : %s\n", WiFi.softAPIP().toString().c_str());
            Serial.printf("  Connected dev.: %d\n", WiFi.softAPgetStationNum());
        } else if (WiFi.status() == WL_CONNECTED) {
            Serial.println("  Mode          : Station (STA)");
            Serial.printf("  Connected to  : %s\n", WiFi.SSID().c_str());
            Serial.printf("  IP address    : %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("  Signal        : %d dBm\n", WiFi.RSSI());
            Serial.printf("  Web interface : http://%s\n", WiFi.localIP().toString().c_str());
            Serial.printf("                  http://%s.local\n", WiFi.getHostname());
        } else {
            Serial.println("  Connection    : NOT CONNECTED");
        }
        printPendingChanges();
        return;
    }

    // ── wifi ssid <wert> ───────────────────────────────────────────────────
    if (strncasecmp(cmd, "ssid ", 5) == 0) {
        const char* val = cmd + 5;
        if (strlen(val) == 0) {
            Serial.println("[Error] SSID must not be empty.");
            return;
        }
        strncpy(pendingSsid, val, sizeof(pendingSsid) - 1);
        pendingSsid[sizeof(pendingSsid) - 1] = '\0';
        Serial.printf("✓ SSID staged: \"%s\" (%d chars)  ->  use 'wifi save' to apply\n", pendingSsid, (int)strlen(pendingSsid));
        return;
    }

    // ── wifi pass <wert> ───────────────────────────────────────────────────
    if (strncasecmp(cmd, "pass ", 5) == 0) {
        const char* val = cmd + 5;
        strncpy(pendingPass, val, sizeof(pendingPass) - 1);
        pendingPass[sizeof(pendingPass) - 1] = '\0';
        Serial.printf("✓ Password staged (%d chars)  ->  use 'wifi save' to apply\n", (int)strlen(pendingPass));
        return;
    }

    // ── wifi host <wert> ───────────────────────────────────────────────────
    if (strncasecmp(cmd, "host ", 5) == 0) {
        const char* val = cmd + 5;
        if (strlen(val) == 0) {
            Serial.println("[Error] Hostname must not be empty.");
            return;
        }
        strncpy(pendingHost, val, sizeof(pendingHost) - 1);
        pendingHost[sizeof(pendingHost) - 1] = '\0';
        Serial.printf("✓ Hostname staged: \"%s\"  ->  use 'wifi save' to apply\n", pendingHost);
        return;
    }

    // ── wifi save ──────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "save") == 0) {
        char ssid[64], pass[64], host[32];
        getWiFiConfig(ssid, pass, host);

        bool anyPending = pendingSsid[0] || pendingPass[0] || pendingHost[0];
        if (!anyPending) {
            Serial.println("[WiFi] No pending changes. Current stored config:");
            Serial.printf("  SSID    : %s\n", ssid[0] ? ssid : "(not set)");
            Serial.printf("  Hostname: %s\n", host);
            Serial.println("  Reconnecting with stored config...");
        } else {
            Serial.println("[WiFi] Applying staged changes:");
            if (pendingSsid[0]) { Serial.printf("  SSID     : \"%s\" -> \"%s\"\n", ssid[0] ? ssid : "(none)", pendingSsid); strncpy(ssid, pendingSsid, sizeof(ssid) - 1); }
            if (pendingPass[0]) { Serial.println("  Password : (updated)"); strncpy(pass, pendingPass, sizeof(pass) - 1); }
            if (pendingHost[0]) { Serial.printf("  Hostname : \"%s\" -> \"%s\"\n", host, pendingHost); strncpy(host, pendingHost, sizeof(host) - 1); }
        }
        ssid[sizeof(ssid)-1] = '\0';
        pass[sizeof(pass)-1] = '\0';
        host[sizeof(host)-1] = '\0';

        setWiFiConfig(ssid, pass, host);

        memset(pendingSsid, 0, sizeof(pendingSsid));
        memset(pendingPass, 0, sizeof(pendingPass));
        memset(pendingHost, 0, sizeof(pendingHost));

        Serial.printf("💾 Saved  ->  SSID: \"%s\"  |  Hostname: \"%s\"\n", ssid[0] ? ssid : "(none)", host);
        Serial.println("   Reconnecting...");
        WiFi.disconnect(true);
        delay(300);
        setupWiFi();
        return;
    }

    // ── wifi reset ─────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "reset") == 0) {
        // Clear saved SSID/password -> device starts in AP mode only
        setWiFiConfig("", "", "D-ActorNode");

        memset(pendingSsid, 0, sizeof(pendingSsid));
        memset(pendingPass, 0, sizeof(pendingPass));
        memset(pendingHost, 0, sizeof(pendingHost));

        Serial.println("🔄 Wi-Fi configuration reset (no network - AP mode active).");
        WiFi.disconnect(true);
        delay(300);
        setupWiFi();
        return;
    }

    // ── wifi ip ─────────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "ip") == 0) {
        if (wifi_ap_mode_active) {
            Serial.printf("[WiFi] AP mode - AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        } else if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] IP address : %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] SSID       : %s\n", WiFi.SSID().c_str());
            Serial.printf("[WiFi] Hostname   : %s\n", WiFi.getHostname());
            Serial.printf("[WiFi] Web        : http://%s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("[WiFi] Not connected.");
        }
        return;
    }

    // ── wifi help ──────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "help") == 0) {
        printHelp();
        return;
    }

    Serial.printf("[Error] Unknown wifi command: 'wifi %s'\n", cmd);
    printHelp();
}

// ─── device commands ──────────────────────────────────────────────────────

static void processDeviceCommand(const char* cmd) {

    // ── device status ──────────────────────────────────────────────────────
    if (strcasecmp(cmd, "status") == 0) {
        DeviceInfo info = getDeviceInfo();
        Serial.println("\n[Device Status]");
        Serial.printf("  Device name (AP-SSID): %s\n", info.deviceName.c_str());
        Serial.printf("  Device number        : %s\n", info.deviceNumber.c_str());
        Serial.printf("  Configuration        : %s\n", info.configuration.c_str());
        if (info.description.length() > 0) {
            Serial.printf("  Description          : %s\n", info.description.c_str());
        }
        return;
    }

    // ── device name <wert> ────────────────────────────────────────────────
    if (strncasecmp(cmd, "name ", 5) == 0) {
        const char* val = cmd + 5;
        if (strlen(val) == 0) {
            Serial.println("[Error] Device name must not be empty.");
            return;
        }
        setDeviceName(val);   // save immediately (NVS)

        // If currently in AP mode, update AP SSID immediately
        if (wifi_ap_mode_active) {
            Serial.printf("   Updating AP-SSID to \"%s\"...\n", val);
            WiFi.softAPdisconnect(false);
            delay(200);
            WiFi.softAP(val);
            delay(200);
            Serial.printf("✓ AP-SSID updated: \"%s\"\n", val);
        } else {
            Serial.printf("✓ Device name set: \"%s\" (takes effect on next AP start)\n", val);
        }
        return;
    }

    // ── device number <wert> ──────────────────────────────────────────────
    if (strncasecmp(cmd, "number ", 7) == 0) {
        const char* val = cmd + 7;
        if (strlen(val) == 0) {
            Serial.println("[Error] Device number must not be empty.");
            return;
        }
        setDeviceNumber(val);
        Serial.printf("✓ Device number set: \"%s\"\n", val);
        return;
    }

    // ── device config <wert> ──────────────────────────────────────────────
    if (strncasecmp(cmd, "config ", 7) == 0) {
        const char* val = cmd + 7;
        setConfiguration(val);
        Serial.printf("✓ Configuration set: \"%s\"\n", val);
        return;
    }

    // ── device desc <wert> ────────────────────────────────────────────────
    if (strncasecmp(cmd, "desc ", 5) == 0) {
        const char* val = cmd + 5;
        setDescription(val);
        Serial.printf("✓ Description set: \"%s\"\n", val);
        return;
    }

    Serial.printf("[Error] Unknown device command: 'device %s'\n", cmd);
    printHelp();
}

// ─── motor commands ───────────────────────────────────────────────────────

static void processMotorCommand(const char* cmd) {

    // ── motor status ──────────────────────────────────────────────────────
    if (strcasecmp(cmd, "status") == 0) {
        Motor28BYJ48Status s = get28BYJ48MotorStatus();
        Serial.println("\n[Motor Status]");
        Serial.printf("  Position : %d steps  (%.1f°)\n", s.currentPosition, s.currentPosition * 360.0f / 4096.0f);
        Serial.printf("  Target   : %d steps  (%.1f°)\n", s.targetPosition, s.targetPosition * 360.0f / 4096.0f);
        Serial.printf("  Moving   : %s\n", s.isMoving ? "Yes" : "No");
        Serial.printf("  Speed    : %d\n", s.currentSpeed);
        Serial.printf("  Homed    : %s\n", s.isHomed ? "Yes" : "No  (run 'motor home' to set reference position)");
        return;
    }

    // ── motor stop ────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "stop") == 0) {
        stop28BYJ48Motor();
        {
            Motor28BYJ48Status s = get28BYJ48MotorStatus();
            Serial.printf("✓ Motor stopped at position %d steps (%.1f°).\n", s.currentPosition, s.currentPosition * 360.0f / 4096.0f);
        }
        return;
    }

    // ── motor home ────────────────────────────────────────────────────────
    if (strcasecmp(cmd, "home") == 0) {
        home28BYJ48Motor();
        {
            Motor28BYJ48Status s = get28BYJ48MotorStatus();
            Serial.printf("✓ Motor at home position (step %d, %.1f°). Homed flag set.\n", s.currentPosition, s.currentPosition * 360.0f / 4096.0f);
        }
        return;
    }

    // ── motor calibrate ───────────────────────────────────────────────────
    if (strcasecmp(cmd, "calibrate") == 0) {
        calibrate28BYJ48Motor();
        {
            Motor28BYJ48Status s = get28BYJ48MotorStatus();
            Serial.printf("✓ Calibration complete. Reference position: %d steps (%.1f°).\n", s.currentPosition, s.currentPosition * 360.0f / 4096.0f);
        }
        return;
    }

    // ── motor speed <value> ───────────────────────────────────────────────
    if (strncasecmp(cmd, "speed ", 6) == 0) {
        int speed = atoi(cmd + 6);
        set28BYJ48MotorSpeed(speed);
        Serial.printf("✓ Motor speed set: %d\n", speed);
        return;
    }

    // ── motor move <steps> <dir> ──────────────────────────────────────────
    if (strncasecmp(cmd, "move ", 5) == 0) {
        int steps = 0, dir = 1;
        if (sscanf(cmd + 5, "%d %d", &steps, &dir) < 1) {
            Serial.println("[Error] Usage: motor move <steps> <dir>");
            return;
        }
        move28BYJ48Motor(steps, dir);
        Serial.printf("✓ Motor moving: %d steps %s  (%.1f°)\n", steps, dir ? "CW" : "CCW", steps * 360.0f / 4096.0f);
        return;
    }

    // ── motor degrees <grad> <dir> ────────────────────────────────────────
    if (strncasecmp(cmd, "degrees ", 8) == 0) {
        float deg = 0.0f;
        int dir = 1;
        if (sscanf(cmd + 8, "%f %d", &deg, &dir) < 1) {
            Serial.println("[Error] Usage: motor degrees <degrees> <dir>");
            return;
        }
        move28BYJ48MotorDegrees(deg, dir);
        Serial.printf("✓ Motor moving: %.1f° %s  (~%d steps)\n", deg, dir ? "CW" : "CCW", (int)(deg * 4096.0f / 360.0f));
        return;
    }

    Serial.printf("[Error] Unknown motor command: 'motor %s'\n", cmd);
    printHelp();
}

// ─── led commands ────────────────────────────────────────────────────────

static void processLedCommand(const char* cmd) {

    // ── led color <r> <g> <b> ─────────────────────────────────────────────
    if (strncasecmp(cmd, "color ", 6) == 0) {
        int r = 0, g = 0, b = 0;
        if (sscanf(cmd + 6, "%d %d %d", &r, &g, &b) < 3) {
            Serial.println("[Error] Usage: led color <r> <g> <b>");
            return;
        }
        setColorRGB(r, g, b);
        Serial.printf("✓ LED color set: R=%-3d G=%-3d B=%-3d  (#%02X%02X%02X)\n", r, g, b, r, g, b);
        return;
    }

    // ── led brightness <value> ───────────────────────────────────────────
    if (strncasecmp(cmd, "brightness ", 11) == 0) {
        int bri = atoi(cmd + 11);
        if (bri < 0) bri = 0;
        if (bri > 255) bri = 255;
        setBrightness(bri);
        Serial.printf("✓ LED brightness: %d / 255  (%.0f%%)\n", bri, bri * 100.0f / 255.0f);
        return;
    }

    // ── led index <n> ─────────────────────────────────────────────────────
    if (strncasecmp(cmd, "index ", 6) == 0) {
        int idx = atoi(cmd + 6);
        setColorByIndex(idx);
        const char* colorNames[] = {"Red", "Green", "Blue", "Yellow", "Purple", "Orange", "White"};
        const char* name = (idx >= 0 && idx < 7) ? colorNames[idx] : "Custom";
        Serial.printf("✓ LED color index: %d  (%s)\n", idx, name);
        return;
    }

    Serial.printf("[Error] Unknown led command: 'led %s'\n", cmd);
    printHelp();
}

// ─── button commands ─────────────────────────────────────────────────────

static void processButtonCommand(const char* cmd) {

    // ── button status ─────────────────────────────────────────────────────
    if (strcasecmp(cmd, "status") == 0) {
        Serial.println("\n[Button Status]");
        Serial.printf("  Enabled : %s\n", isButtonEnabled() ? "Yes" : "No");
        Serial.printf("  Pressed : %s\n", getButtonState() ? "Yes" : "No");
        return;
    }

    // ── button enable ─────────────────────────────────────────────────────
    if (strcasecmp(cmd, "enable") == 0) {
        setButtonEnabled(true);
        Serial.println("✓ Button enabled.");
        return;
    }

    // ── button disable ────────────────────────────────────────────────────
    if (strcasecmp(cmd, "disable") == 0) {
        setButtonEnabled(false);
        Serial.println("✓ Button disabled.");
        return;
    }

    Serial.printf("[Error] Unknown button command: 'button %s'\n", cmd);
    printHelp();
}

// ─── Main dispatcher ──────────────────────────────────────────────────────

static void processCommand(const char* line) {
    if (strncasecmp(line, "wifi ", 5) == 0) {
        processWifiCommand(line + 5);
        return;
    }
    if (strncasecmp(line, "device ", 7) == 0) {
        processDeviceCommand(line + 7);
        return;
    }
    if (strncasecmp(line, "motor ", 6) == 0) {
        processMotorCommand(line + 6);
        return;
    }
    if (strncasecmp(line, "led ", 4) == 0) {
        processLedCommand(line + 4);
        return;
    }
    if (strncasecmp(line, "button ", 7) == 0) {
        processButtonCommand(line + 7);
        return;
    }
    if (strcasecmp(line, "help") == 0) {
        printHelp();
        return;
    }
    if (strncasecmp(line, "registry ", 9) == 0) {
        // ── registry list ──────────────────────────────────────────────────────
        const char* sub = line + 9;
        if (strcasecmp(sub, "list") == 0) {
            int n = getDeviceCount();
            Serial.printf("\n[Registry] %d device(s)\n", n);
            Serial.println(getDeviceListJson());
            return;
        }
        // ── registry clear ────────────────────────────────────────────────
        if (strcasecmp(sub, "clear") == 0) {
            clearDeviceRegistry();
            Serial.println("✓ All registry entries cleared.");
            return;
        }
        // ── registry remove <id> ─────────────────────────────────────────
        if (strncasecmp(sub, "remove ", 7) == 0) {
            uint16_t id = (uint16_t)atoi(sub + 7);
            if (removeDevice(id)) {
                Serial.printf("✓ Device %d removed (takes effect after reboot)\n", id);
            } else {
                Serial.printf("[Error] No device with ID %d\n", id);
            }
            return;
        }
        // ── registry add <typ> <name> [k=v ...] ────────────────────────
        if (strncasecmp(sub, "add ", 4) == 0) {
            char buf[220];
            strncpy(buf, sub + 4, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';

            // Tokenize: <type> <name> [key=value ...]
            char* saveptr = nullptr;
            char* typStr  = strtok_r(buf,  " ", &saveptr);
            char* nameStr = strtok_r(nullptr, " ", &saveptr);
            if (!typStr || !nameStr) {
                Serial.println("[Error] Usage: registry add <type> <name> [key=value ...]");
                return;
            }
            DeviceType dtype = deviceTypeFromString(typStr);
            if (dtype == DEV_UNKNOWN) {
                Serial.printf("[Error] Unknown type: '%s'\n", typStr);
                return;
            }

            // Restliche Token als key=value -> minimales JSON bauen
            String jsonCfg = "{";
            bool first = true;
            char* token;
            while ((token = strtok_r(nullptr, " ", &saveptr)) != nullptr) {
                char* eq = strchr(token, '=');
                if (!eq) continue;
                *eq = '\0';
                const char* key = token;
                const char* val = eq + 1;
                if (!first) jsonCfg += ",";
                first = false;
                // Zahlen nicht quoten
                bool isNum = true;
                for (const char* c = val; *c; c++)
                    if (!isdigit((unsigned char)*c) && *c != '.' && *c != '-') { isNum = false; break; }
                jsonCfg += String("\"") + key + "\":"
                         + (isNum ? String(val) : String("\"") + val + "\"");
            }
            jsonCfg += "}";

            int newId = addDevice(dtype, nameStr, jsonCfg.c_str());
            if (newId < 0) {
                Serial.println("[Error] Registry full.");
            } else {
                Serial.printf("✓ Device created: id=%d type=%s name=%s\n",
                              newId, deviceTypeToString(dtype), nameStr);
                Serial.printf("  Configuration : %s\n", jsonCfg.c_str());
                Serial.println("  -> Restart to activate hardware.");
            }
            return;
        }
        Serial.println("[Error] Unknown registry command. See 'help'.");
        return;
    }
    // Silently ignore all other input (other modules may use UART)
}

// ─── Public functions ─────────────────────────────────────────────────────

void setupUartWifiConfig() {
    Serial.println();
    Serial.println("[System] Verbunden \u2013 115200 Baud, 8N1.");
    Serial.println("[System] Tipp: \"wifi status\" oder \"device status\" eingeben.");
    printHelp();
}

void handleUartWifiConfig() {
    while (Serial.available()) {
        char c = (char)Serial.read();

        if (c == '\r') continue;  // Ignore Windows CRLF

        if (c == '\n' || inputPos >= (int)sizeof(inputBuffer) - 1) {
            inputBuffer[inputPos] = '\0';
            if (inputPos > 0) {
                Serial.printf("\n> %s\n", inputBuffer);
                processCommand(inputBuffer);
            }
            inputPos = 0;
        } else {
            inputBuffer[inputPos++] = c;
        }
    }
}

