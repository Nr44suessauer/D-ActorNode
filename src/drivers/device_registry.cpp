#include "device_registry.h"
#include "button_control.h"
#include "led_control.h"
#include "motor_28byj48.h"
#include "servo_control.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// ─── Interner Zustand ─────────────────────────────────────────────────────────

static DeviceEntry s_registry[DEVICE_REGISTRY_MAX];
static uint16_t    s_nextId = 1;

static Preferences s_prefs;
#define NVS_NAMESPACE "dev_registry"
#define NVS_KEY_COUNT "count"
#define NVS_KEY_NEXT  "nextid"

// ─── Hilfsfunktionen ──────────────────────────────────────────────────────────

static String entryKey(int slot) {
    return String("e") + String(slot);
}

static void saveEntry(int slot) {
    if (!s_registry[slot].active) return;
    JsonDocument doc;
    doc["id"]     = s_registry[slot].id;
    doc["type"]   = (int)s_registry[slot].type;
    doc["name"]   = s_registry[slot].name;
    doc["config"] = s_registry[slot].config;
    String out;
    serializeJson(doc, out);
    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.putString(entryKey(slot).c_str(), out);
    s_prefs.putUInt(NVS_KEY_NEXT, s_nextId);
    s_prefs.end();
}

static void deleteEntry(int slot) {
    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.remove(entryKey(slot).c_str());
    s_prefs.end();
}

static void saveCount() {
    s_prefs.begin(NVS_NAMESPACE, false);
    s_prefs.putUInt(NVS_KEY_NEXT, s_nextId);
    s_prefs.end();
}

// ─── Boot-Init der registrierten Hardware ────────────────────────────────────

void bootInitRegisteredDevices() {
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        if (!s_registry[i].active) continue;

        JsonDocument cfg;
        deserializeJson(cfg, s_registry[i].config);

        switch (s_registry[i].type) {

            case DEV_BUTTON: {
                int pin     = cfg["pin"]    | -1;
                bool pullup = cfg["pullup"] | true;
                if (pin >= 0) {
                    pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
                    Serial.printf("[Registry] Button '%s' -> GPIO%d\n",
                                  s_registry[i].name, pin);
                }
                break;
            }

            case DEV_LED: {
                int pin      = cfg["pin"]        | -1;
                int count    = cfg["count"]      | 1;
                int brightness = cfg["brightness"] | 5;
                if (pin >= 0) {
                    // NeoPixel via existing LED driver (output slot 2 upwards)
                    Serial.printf("[Registry] LED '%s' -> GPIO%d, count=%d\n",
                                  s_registry[i].name, pin, count);
                    // Runtime re-init not supported by current LED driver;
                    // config is available for next reboot with setupLEDs().
                }
                break;
            }

            case DEV_MOTOR: {
                int p1 = cfg["pin1"] | -1;
                int p2 = cfg["pin2"] | -1;
                int p3 = cfg["pin3"] | -1;
                int p4 = cfg["pin4"] | -1;
                if (p1 >= 0 && p2 >= 0 && p3 >= 0 && p4 >= 0) {
                    setup28BYJ48MotorWithPins(p1, p2, p3, p4);
                    Serial.printf("[Registry] Motor '%s' -> GPIO%d,%d,%d,%d\n",
                                  s_registry[i].name, p1, p2, p3, p4);
                }
                break;
            }

            case DEV_SERVO: {
                int pin    = cfg["pin"]    | -1;
                int minUs  = cfg["min_us"] | 500;
                int maxUs  = cfg["max_us"] | 2500;
                if (pin >= 0) {
                    Serial.printf("[Registry] Servo '%s' -> GPIO%d (%d-%d µs)\n",
                                  s_registry[i].name, pin, minUs, maxUs);
                    // setServoPin(0, pin) could be called here; kept as config-only
                }
                break;
            }

            default:
                break;
        }
    }
}

// ─── Public API ───────────────────────────────────────────────────────────────

void initDeviceRegistry() {
    memset(s_registry, 0, sizeof(s_registry));
    s_nextId = 1;

    s_prefs.begin(NVS_NAMESPACE, true);
    s_nextId = s_prefs.getUInt(NVS_KEY_NEXT, 1);

    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        String raw = s_prefs.getString(entryKey(i).c_str(), "");
        if (raw.length() == 0) continue;

        JsonDocument doc;
        if (deserializeJson(doc, raw) != DeserializationError::Ok) continue;

        s_registry[i].active = true;
        s_registry[i].id     = doc["id"]   | (uint16_t)0;
        s_registry[i].type   = (DeviceType)(doc["type"] | (int)DEV_UNKNOWN);
        strncpy(s_registry[i].name,   doc["name"]   | "", sizeof(s_registry[i].name)   - 1);
        strncpy(s_registry[i].config, doc["config"] | "", sizeof(s_registry[i].config) - 1);
        Serial.printf("[Registry] Loaded: id=%d type=%s name=%s\n",
                      s_registry[i].id,
                      deviceTypeToString(s_registry[i].type),
                      s_registry[i].name);
    }
    s_prefs.end();

    bootInitRegisteredDevices();
}

int getDeviceCount() {
    int c = 0;
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++)
        if (s_registry[i].active) c++;
    return c;
}

const DeviceEntry* getDeviceById(uint16_t id) {
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++)
        if (s_registry[i].active && s_registry[i].id == id)
            return &s_registry[i];
    return nullptr;
}

String getDeviceListJson() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        if (!s_registry[i].active) continue;
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]   = s_registry[i].id;
        obj["type"] = deviceTypeToString(s_registry[i].type);
        obj["name"] = s_registry[i].name;

        // Inline config object
        JsonDocument cfg;
        if (deserializeJson(cfg, s_registry[i].config) == DeserializationError::Ok) {
            obj["config"] = cfg.as<JsonObject>();
        }
    }
    String out;
    serializeJson(doc, out);
    return out;
}

int addDevice(DeviceType type, const char* name, const char* configJson) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        if (!s_registry[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1;  // voll

    s_registry[slot].active = true;
    s_registry[slot].id     = s_nextId++;
    s_registry[slot].type   = type;
    strncpy(s_registry[slot].name,   name,       sizeof(s_registry[slot].name)   - 1);
    strncpy(s_registry[slot].config, configJson, sizeof(s_registry[slot].config) - 1);
    s_registry[slot].name  [sizeof(s_registry[slot].name)   - 1] = '\0';
    s_registry[slot].config[sizeof(s_registry[slot].config) - 1] = '\0';

    saveEntry(slot);
    saveCount();

    Serial.printf("[Registry] Neu: id=%d type=%s name=%s\n",
                  s_registry[slot].id, deviceTypeToString(type), name);
    return (int)s_registry[slot].id;
}

bool removeDevice(uint16_t id) {
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        if (s_registry[i].active && s_registry[i].id == id) {
            s_registry[i].active = false;
            deleteEntry(i);
            Serial.printf("[Registry] Entfernt: id=%d\n", id);
            return true;
        }
    }
    return false;
}

void clearDeviceRegistry() {
    s_prefs.begin(NVS_NAMESPACE, false);
    for (int i = 0; i < DEVICE_REGISTRY_MAX; i++) {
        if (s_registry[i].active) {
            s_prefs.remove(entryKey(i).c_str());
            s_registry[i].active = false;
        }
    }
    s_nextId = 1;
    s_prefs.putUInt(NVS_KEY_NEXT, s_nextId);
    s_prefs.end();
    Serial.println("[Registry] All entries cleared.");
}

DeviceType deviceTypeFromString(const char* s) {
    if (strcasecmp(s, "button")  == 0) return DEV_BUTTON;
    if (strcasecmp(s, "led")     == 0) return DEV_LED;
    if (strcasecmp(s, "motor")   == 0) return DEV_MOTOR;
    if (strcasecmp(s, "servo")   == 0) return DEV_SERVO;
    if (strcasecmp(s, "stepper") == 0) return DEV_STEPPER;
    return DEV_UNKNOWN;
}

const char* deviceTypeToString(DeviceType t) {
    switch (t) {
        case DEV_BUTTON:  return "button";
        case DEV_LED:     return "led";
        case DEV_MOTOR:   return "motor";
        case DEV_SERVO:   return "servo";
        case DEV_STEPPER: return "stepper";
        default:          return "unknown";
    }
}
