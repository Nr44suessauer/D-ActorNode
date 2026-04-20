#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

#include <Arduino.h>

// Button pin definition
#define BUTTON_PIN 45     // Button input pin

// Function prototypes
void setupButton();

/**
 * Returns the current (interrupt-based) button state.
 * LOW (pressed) = true, HIGH (not pressed) = false.
 * Always returns false when the button is disabled.
 */
bool getButtonState();

/** Enables or disables the button interrupt. Saved to NVS. */
void setButtonEnabled(bool enabled);

/** Returns whether the button is enabled. */
bool isButtonEnabled();

#endif // BUTTON_CONTROL_H