#include "advanced_motor.h"
#include "button_control.h"  // For button state query
#include "driver/gpio.h"

// Globale Debug-Variable
bool motorDebugEnabled = false;  // Disabled by default

// Multi-motor pin configuration defaults
int ADV_MOTOR_STEP_PINS[MAX_ADVANCED_MOTORS] = {STEP_PIN, 35, 33};
int ADV_MOTOR_DIR_PINS[MAX_ADVANCED_MOTORS] = {DIR_PIN, 34, 36};
int ADV_MOTOR_ENABLE_PINS[MAX_ADVANCED_MOTORS] = {ENABLE_PIN, -1, -1};

// Globale Instanzen
AdvancedStepperMotor advancedMotor(STEP_PIN, DIR_PIN, ENABLE_PIN, ADVANCED_STEPS_PER_REVOLUTION);
AdvancedStepperMotor advancedMotor2(35, 34, -1, ADVANCED_STEPS_PER_REVOLUTION);
AdvancedStepperMotor advancedMotor3(33, 36, -1, ADVANCED_STEPS_PER_REVOLUTION);

static bool isValidOutputPin(int pin) {
    return pin >= 0 && GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(pin));
}

// Konstruktor
AdvancedStepperMotor::AdvancedStepperMotor(int stepPin, int dirPin, int enablePin, int stepsPerRevolution)
    : stepPin(stepPin), dirPin(dirPin), enablePin(enablePin), stepsPerRevolution(stepsPerRevolution) {
    currentPosition = 0;
    targetPosition = 0;
    virtualHomePosition = 0; // Initialize virtual home position
    isMoving = false;
    isEnabled = false;
    isHomed = false;
    currentSpeedRPM = DEFAULT_SPEED_RPM;
    stepDelayMicros = 0;
    lastStepTime = 0;
    targetPassCount = 0;
    currentPassCount = 0;
    isPassingButton = false;
    calculateStepDelay();
}

void AdvancedStepperMotor::begin() {
    if (!isValidOutputPin(stepPin) || !isValidOutputPin(dirPin) || (enablePin >= 0 && !isValidOutputPin(enablePin))) {
        MOTOR_DEBUG_PRINTF("Invalid motor pins STEP=%d DIR=%d EN=%d - motor disabled\n", stepPin, dirPin, enablePin);
        isEnabled = false;
        isMoving = false;
        return;
    }

    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    if (enablePin >= 0) {
        pinMode(enablePin, OUTPUT);
        digitalWrite(enablePin, HIGH);
    }
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, LOW);
    enable();
    MOTOR_DEBUG_PRINTLN("Advanced motor initialized");
}

void AdvancedStepperMotor::reconfigurePins(int newStepPin, int newDirPin, int newEnablePin) {
    disable();

    // Keep previous valid pins if new values are invalid.
    if (isValidOutputPin(newStepPin)) {
        stepPin = newStepPin;
    }
    if (isValidOutputPin(newDirPin)) {
        dirPin = newDirPin;
    }
    if (newEnablePin == -1 || isValidOutputPin(newEnablePin)) {
        enablePin = newEnablePin;
    }

    begin();
    setSpeed(currentSpeedRPM);
    enable();
}

void AdvancedStepperMotor::enable() {
    if (enablePin >= 0) digitalWrite(enablePin, LOW);
    isEnabled = true;
}

void AdvancedStepperMotor::disable() {
    if (enablePin >= 0) digitalWrite(enablePin, HIGH);
    isEnabled = false;
    isMoving = false;
    setPinsIdle();
}

void AdvancedStepperMotor::setPinsIdle() {
    digitalWrite(stepPin, LOW);
    digitalWrite(dirPin, LOW);
}

void AdvancedStepperMotor::setSpeed(int rpm) {
    rpm = constrain(rpm, 1, MAX_SPEED_RPM);
    currentSpeedRPM = rpm;
    calculateStepDelay();
    MOTOR_DEBUG_PRINTF("Motor speed: %d RPM\n", rpm);
}

void AdvancedStepperMotor::calculateStepDelay() {
    stepDelayMicros = (60000000L) / (currentSpeedRPM * stepsPerRevolution * 2);
}

void AdvancedStepperMotor::setDirection(bool clockwise) {
    digitalWrite(dirPin, clockwise ? HIGH : LOW);
    delayMicroseconds(5);
}

void AdvancedStepperMotor::step() {
    if (!isEnabled) return;
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelayMicros);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelayMicros);
}

void AdvancedStepperMotor::moveSteps(int steps) {
    if (!isEnabled || steps == 0) return;
    isMoving = true;
    setDirection(steps > 0);
    int absSteps = abs(steps);
    for (int i = 0; i < absSteps; i++) {
        step();
    }
    currentPosition += steps;
    targetPosition = currentPosition;
    isMoving = false;
    setPinsIdle();
}

void AdvancedStepperMotor::moveTo(int position) {
    moveSteps(position - currentPosition);
}

void AdvancedStepperMotor::moveRelative(int steps) {
    moveSteps(steps);
}

void AdvancedStepperMotor::stop() {
    isMoving = false;
    setPinsIdle();
}

void AdvancedStepperMotor::setHome() {
    currentPosition = 0;
    targetPosition = 0;
    isHomed = true;
}

void AdvancedStepperMotor::homeToButton() {
    if (!isEnabled) {
        MOTOR_DEBUG_PRINTLN("Motor disabled - homing aborted");
        return;
    }
    
    MOTOR_DEBUG_PRINTLN("Starting homing to button at current speed: " + String(currentSpeedRPM) + " RPM");
    MOTOR_DEBUG_PRINTLN("Current button state: " + String(getButtonState() == HIGH ? "HIGH (not pressed)" : "LOW (pressed)"));
    MOTOR_DEBUG_PRINTLN("Current motor position: " + String(currentPosition));
    
    // Check if button is already pressed
    if (getButtonState() == LOW) {
        MOTOR_DEBUG_PRINTLN("Button already pressed - setting current position as home");
        currentPosition = 0;
        targetPosition = 0;
        isHomed = true;
        return;
    }
    
    // Use current parameters from slider (speed is already set)
    // Additional parameters can be added here if needed
    
    // Drive toward button (negative direction assumed)
    // The button acts as home limit switch
    isMoving = true;
    setDirection(false);  // Move in negative direction toward button
    
    int stepCount = 0;
    int maxSteps = 10000;  // Safety maximum to avoid endless movement
    
    MOTOR_DEBUG_PRINTLN("Starting movement in negative direction...");
    
    // Move until button is pressed or maximum is reached
    while (getButtonState() == HIGH && stepCount < maxSteps && isMoving) {
        // Button not pressed (HIGH = not pressed with INPUT_PULLUP)
        step();
        currentPosition++;  // Position is decremented on negative travel
        stepCount++;
        
        // Short pause for button query and debug output
        if (stepCount % 30 == 0) {
            MOTOR_DEBUG_PRINTLN("Steps: " + String(stepCount) + ", Button: " + String(getButtonState() == HIGH ? "HIGH" : "LOW") + ", Position: " + String(currentPosition));
            delay(1);  // Slightly longer pause for button query
        }
    }
    
    // Check result
    if (getButtonState() == LOW) {
        // Button pressed (LOW = pressed with INPUT_PULLUP)
        MOTOR_DEBUG_PRINTLN("Home position reached at button after " + String(stepCount) + " steps");
        currentPosition = 0;  // Set current position as home (0)
        targetPosition = 0;
        isHomed = true;
        MOTOR_DEBUG_PRINTLN("Motor successfully homed - new home position set");
    } else if (stepCount >= maxSteps) {
        MOTOR_DEBUG_PRINTLN("WARNING: Maximum step count reached - button not found!");
        MOTOR_DEBUG_PRINTLN("Button may be defective or travel direction wrong");
        MOTOR_DEBUG_PRINTLN("Current button state: " + String(getButtonState() == HIGH ? "HIGH (not pressed)" : "LOW (pressed)"));
    } else if (!isMoving) {
        MOTOR_DEBUG_PRINTLN("Homing aborted");
    }
    
    isMoving = false;
    setPinsIdle();
    MOTOR_DEBUG_PRINTLN("Homing complete. Current position: " + String(currentPosition));
}

void AdvancedStepperMotor::passButtonTimes(int count) {
    if (!isEnabled) {
        MOTOR_DEBUG_PRINTLN("Motor disabled - button pass run aborted");
        return;
    }
    
    if (count <= 0) {
        MOTOR_DEBUG_PRINTLN("Invalid count: " + String(count) + " - button pass run aborted");
        return;
    }
    
    // Initialisiere Passier-Variablen
    targetPassCount = count;
    currentPassCount = 0;
    isPassingButton = true;
    
    MOTOR_DEBUG_PRINTLN("Starting button pass run: button should be passed " + String(count) + " times");
    MOTOR_DEBUG_PRINTLN("Speed: " + String(currentSpeedRPM) + " RPM");
    MOTOR_DEBUG_PRINTLN("Current button state: " + String(getButtonState() == HIGH ? "HIGH (not pressed)" : "LOW (pressed)"));
    MOTOR_DEBUG_PRINTLN("Start position: " + String(currentPosition));
    
    isMoving = true;
    setDirection(false);  // Move in negative direction (same direction as homeToButton)
    
    int passedCount = 0;
    int stepCount = 0;
    // maxSteps removed - no step limit anymore
    bool lastButtonState = (getButtonState() == LOW);  // true when button pressed
    bool currentButtonState;
    
    MOTOR_DEBUG_PRINTLN("Starting run - target: " + String(count) + " button passes (no step limit)");
    
    // Run until desired number of button passes is reached
    while (passedCount < count && isMoving) {
        // Take one step
        step();
        currentPosition--;  // Position decremented on negative travel
        stepCount++;
        
        // Check button state
        currentButtonState = (getButtonState() == LOW);
        
        // Check for edge: from not-pressed to pressed (rising edge of pass)
        if (!lastButtonState && currentButtonState) {
            passedCount++;
            currentPassCount = passedCount; // Update the class variable
            MOTOR_DEBUG_PRINTLN("Button pass " + String(passedCount) + " of " + String(count) + 
                              " at step " + String(stepCount) + ", position: " + String(currentPosition));
            

        }
        
        lastButtonState = currentButtonState;
        
        // Debug output every 50 steps for better feedback
        if (stepCount % 50 == 0) {
            MOTOR_DEBUG_PRINTLN("Steps: " + String(stepCount) + 
                              ", passes: " + String(passedCount) + "/" + String(count) + 
                              ", button: " + String(getButtonState() == HIGH ? "HIGH" : "LOW") + 
                              ", position: " + String(currentPosition));
            delay(5);  // Slightly longer pause for UI update opportunity
        }
    }
    
    // Evaluate result
    if (passedCount >= count) {
        MOTOR_DEBUG_PRINTLN("SUCCESS: Button passed " + String(passedCount) + " times");
        MOTOR_DEBUG_PRINTLN("Steps required: " + String(stepCount));
        MOTOR_DEBUG_PRINTLN("End-Position: " + String(currentPosition));
    } else if (!isMoving) {
        MOTOR_DEBUG_PRINTLN("Movement was stopped");
        MOTOR_DEBUG_PRINTLN("Reached passes: " + String(passedCount) + " of " + String(count));
    }
    
    isMoving = false;
    isPassingButton = false;
    setPinsIdle();
    MOTOR_DEBUG_PRINTLN("Button pass run complete. End position: " + String(currentPosition));
}

void AdvancedStepperMotor::setVirtualHome() {
    virtualHomePosition = currentPosition;
    isHomed = true;
    MOTOR_DEBUG_PRINTLN("Virtual home position set to: " + String(virtualHomePosition));
    MOTOR_DEBUG_PRINTLN("Current position: " + String(currentPosition));
}

void AdvancedStepperMotor::moveToVirtualHome() {
    if (!isEnabled) {
        MOTOR_DEBUG_PRINTLN("Motor disabled - movement to virtual home aborted");
        return;
    }
    
    MOTOR_DEBUG_PRINTLN("Moving to virtual home position: " + String(virtualHomePosition));
    MOTOR_DEBUG_PRINTLN("Current position: " + String(currentPosition));
    MOTOR_DEBUG_PRINTLN("Speed: " + String(currentSpeedRPM) + " RPM");
    
    int stepsToVirtualHome = virtualHomePosition - currentPosition;
    MOTOR_DEBUG_PRINTLN("Steps required: " + String(stepsToVirtualHome));
    
    if (stepsToVirtualHome == 0) {
        MOTOR_DEBUG_PRINTLN("Already at virtual home position");
        return;
    }
    
    moveRelative(stepsToVirtualHome);
    MOTOR_DEBUG_PRINTLN("Movement to virtual home position complete");
}

int AdvancedStepperMotor::getCurrentPosition() { return currentPosition; }
int AdvancedStepperMotor::getTargetPosition() { return targetPosition; }
int AdvancedStepperMotor::getVirtualHomePosition() { return virtualHomePosition; }
bool AdvancedStepperMotor::getIsMoving() { return isMoving; }
bool AdvancedStepperMotor::getIsEnabled() { return isEnabled; }
int AdvancedStepperMotor::getCurrentSpeed() { return currentSpeedRPM; }
bool AdvancedStepperMotor::getIsHomed() { return isHomed; }
int AdvancedStepperMotor::getTargetPassCount() { return targetPassCount; }
int AdvancedStepperMotor::getCurrentPassCount() { return currentPassCount; }
bool AdvancedStepperMotor::getIsPassingButton() { return isPassingButton; }

AdvancedMotorStatus AdvancedStepperMotor::getStatus() {
    AdvancedMotorStatus status;
    status.currentPosition = currentPosition;
    status.targetPosition = targetPosition;
    status.virtualHomePosition = virtualHomePosition;
    status.isMoving = isMoving;
    status.currentSpeed = currentSpeedRPM;
    status.isHomed = isHomed;
    status.isEnabled = isEnabled;
    status.targetPassCount = targetPassCount;
    status.currentPassCount = currentPassCount;
    status.isPassingButton = isPassingButton;
    return status;
}

void AdvancedStepperMotor::update() {
    if (!isMoving) return;
    
    unsigned long currentTime = micros();
    if (currentTime - lastStepTime >= stepDelayMicros) {
        if (currentPosition != targetPosition) {
            step();
            lastStepTime = currentTime;
        } else {
            isMoving = false;
        }
    }
}

// Globale Setup-Funktion
void setupAdvancedMotor() {
    advancedMotor.reconfigurePins(ADV_MOTOR_STEP_PINS[0], ADV_MOTOR_DIR_PINS[0], ADV_MOTOR_ENABLE_PINS[0]);
    advancedMotor2.reconfigurePins(ADV_MOTOR_STEP_PINS[1], ADV_MOTOR_DIR_PINS[1], ADV_MOTOR_ENABLE_PINS[1]);
    advancedMotor3.reconfigurePins(ADV_MOTOR_STEP_PINS[2], ADV_MOTOR_DIR_PINS[2], ADV_MOTOR_ENABLE_PINS[2]);

    advancedMotor.setSpeed(60);
    advancedMotor2.setSpeed(60);
    advancedMotor3.setSpeed(60);

    advancedMotor.enable();
    advancedMotor2.enable();
    advancedMotor3.enable();
}

// Global update function for main loop
void updateMotor() {
    advancedMotor.update();
    advancedMotor2.update();
    advancedMotor3.update();
}

AdvancedStepperMotor& getAdvancedMotorById(uint8_t motorId) {
    if (motorId == 2) return advancedMotor2;
    if (motorId == 3) return advancedMotor3;
    return advancedMotor;
}

bool configureAdvancedMotorPinsById(uint8_t motorId, int stepPin, int dirPin, int enablePin) {
    if (motorId < 1 || motorId > MAX_ADVANCED_MOTORS) return false;

    ADV_MOTOR_STEP_PINS[motorId - 1] = stepPin;
    ADV_MOTOR_DIR_PINS[motorId - 1] = dirPin;
    ADV_MOTOR_ENABLE_PINS[motorId - 1] = enablePin;

    AdvancedStepperMotor& motor = getAdvancedMotorById(motorId);
    motor.reconfigurePins(stepPin, dirPin, enablePin);
    return true;
}

// Global home function with button
void homeMotorToButton() {
    advancedMotor.homeToButton();
}

// Global function for button passes
void passButtonTimes(int count) {
    advancedMotor.passButtonTimes(count);
}

// Global calibration function for virtual home position
void calibrateVirtualHome() {
    advancedMotor.setVirtualHome();
}

// Global function for movement to virtual home position
void moveToVirtualHome() {
    advancedMotor.moveToVirtualHome();
}

// Debug-Funktionen
void setMotorDebug(bool enabled) {
    motorDebugEnabled = enabled;
    if (enabled) {
        MOTOR_DEBUG_PRINTLN("Motor-Debug aktiviert");
    }
}

bool getMotorDebugStatus() {
    return motorDebugEnabled;
}
