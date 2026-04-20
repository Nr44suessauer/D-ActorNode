#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <Arduino.h>

// ─── Typen ────────────────────────────────────────────────────────────────────

typedef enum : uint8_t {
    DEV_BUTTON   = 0,
    DEV_LED      = 1,
    DEV_MOTOR    = 2,   // 28BYJ-48
    DEV_SERVO    = 3,
    DEV_STEPPER  = 4,   // NEMA/A4988
    DEV_UNKNOWN  = 255
} DeviceType;

// Maximum number of total instances (observe NVS key limit)
#define DEVICE_REGISTRY_MAX 32
// Maximum length of JSON configuration string per device
#define DEVICE_CONFIG_MAX_LEN 256

typedef struct {
    bool      active;
    uint16_t  id;
    DeviceType type;
    char      name[32];
    char      config[DEVICE_CONFIG_MAX_LEN];  // JSON-String, typspezifisch
} DeviceEntry;

// ─── Public API ──────────────────────────────────────────────────────────────

/**
 * Loads registry from NVS and initializes all stored devices.
 * Must be called in setup().
 */
void initDeviceRegistry();

/**
 * Returns the number of active entries.
 */
int getDeviceCount();

/**
 * Returns a pointer to the entry with the given ID,
 * or nullptr if not found.
 */
const DeviceEntry* getDeviceById(uint16_t id);

/**
 * Returns all active entries as a JSON string.
 * The caller does not need to free the string (managed internally).
 */
String getDeviceListJson();

/**
 * Creates a new device. config is a JSON string with
 * type-specific parameters.
 * Returns the new ID, or -1 on error.
 */
int addDevice(DeviceType type, const char* name, const char* configJson);

/**
 * Removes a device by its ID.
 * Returns true if successful.
 */
bool removeDevice(uint16_t id);

/**
 * Deletes all devices from the registry and NVS.
 */
void clearDeviceRegistry();

/**
 * Converts a type string (e.g. "button") to DeviceType.
 */
DeviceType deviceTypeFromString(const char* s);

/**
 * Converts DeviceType to a human-readable string.
 */
const char* deviceTypeToString(DeviceType t);

// ─── Boot initialization of registered hardware ───────────────────────────

/**
 * Initializes all stored devices (GPIO setup, driver init).
 * Called internally by initDeviceRegistry().
 */
void bootInitRegisteredDevices();

#endif // DEVICE_REGISTRY_H
