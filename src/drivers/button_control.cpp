#include "button_control.h"
#include "driver/gpio.h"
#include <Preferences.h>

// ─── Interrupt state ─────────────────────────────────────────────────────────
static volatile bool     isr_triggered  = false;   // set by ISR
static volatile uint32_t isr_time_ms    = 0;        // timestamp of last ISR trigger

static bool buttonState   = HIGH;  // current stable state (HIGH = not pressed)
static bool buttonEnabled = true;  // button enabled?

static const uint32_t DEBOUNCE_MS = 25;

// NVS namespace for button settings
static Preferences buttonPrefs;

// ─── ISR ─────────────────────────────────────────────────────────────────────
static void IRAM_ATTR buttonISR() {
    isr_triggered = true;
    isr_time_ms   = (uint32_t)millis();
}

// ─── setupButton ─────────────────────────────────────────────────────────────
void setupButton() {
    if (!GPIO_IS_VALID_GPIO(static_cast<gpio_num_t>(BUTTON_PIN))) {
        Serial.println("[Button] GPIO invalid, init skipped");
        return;
    }

    // Lade gespeicherten enabled-Status
    buttonPrefs.begin("button_cfg", true);
    buttonEnabled = buttonPrefs.getBool("enabled", true);
    buttonPrefs.end();

    pinMode(BUTTON_PIN, INPUT);
    buttonState = (bool)digitalRead(BUTTON_PIN);

    if (buttonEnabled) {
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);
        Serial.printf("[Button] Pin %d initialized (interrupt, %s)\n",
                      BUTTON_PIN, buttonState == LOW ? "pressed" : "not pressed");
    } else {
        Serial.printf("[Button] Pin %d initialized (DISABLED)\n", BUTTON_PIN);
    }
}

// ─── getButtonState ───────────────────────────────────────────────────────────
bool getButtonState() {
    if (!buttonEnabled) return false;

    // ISR reported an edge -> evaluate after debounce time
    if (isr_triggered && ((uint32_t)millis() - isr_time_ms) >= DEBOUNCE_MS) {
        isr_triggered    = false;
        bool newState    = (bool)digitalRead(BUTTON_PIN);
        if (newState != buttonState) {
            buttonState = newState;
            Serial.printf("[Button] State: %s\n", buttonState == LOW ? "PRESSED" : "released");
        }
    }

    return buttonState == LOW;   // LOW = pressed -> true
}

// ─── setButtonEnabled ────────────────────────────────────────────────────────
void setButtonEnabled(bool enabled) {
    buttonEnabled = enabled;

    if (enabled) {
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);
        isr_triggered = false;
        buttonState   = (bool)digitalRead(BUTTON_PIN);
        Serial.println("[Button] Enabled (interrupt attached)");
    } else {
        detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));
        Serial.println("[Button] Disabled (interrupt removed)");
    }

    // Persistieren
    buttonPrefs.begin("button_cfg", false);
    buttonPrefs.putBool("enabled", enabled);
    buttonPrefs.end();
}

// ─── isButtonEnabled ─────────────────────────────────────────────────────────
bool isButtonEnabled() {
    return buttonEnabled;
}