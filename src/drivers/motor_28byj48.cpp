#include "motor_28byj48.h"
#include "pin_config.h"

// Global pin variables (can be changed at runtime)
int motor_28byj48_pin_1 = MOTOR_28BYJ48_DEFAULT_PIN_1;
int motor_28byj48_pin_2 = MOTOR_28BYJ48_DEFAULT_PIN_2;
int motor_28byj48_pin_3 = MOTOR_28BYJ48_DEFAULT_PIN_3;
int motor_28byj48_pin_4 = MOTOR_28BYJ48_DEFAULT_PIN_4;

/**
 * @brief Half-step sequence for 28BYJ-48 stepper motor control.
 */
int motor_28byj48_sequence[8][4] = {
    {1, 0, 0, 0},  // Step 1
    {1, 1, 0, 0},  // Step 2
    {0, 1, 0, 0},  // Step 3
    {0, 1, 1, 0},  // Step 4
    {0, 0, 1, 0},  // Step 5
    {0, 0, 1, 1},  // Step 6
    {0, 0, 0, 1},  // Step 7
    {1, 0, 0, 1}   // Step 8
};

// Variable to store the current motor position
int current_28byj48_motor_position = 0;
int current_28byj48_step_index = 0;
bool motor_28byj48_is_moving = false;
int current_28byj48_motor_speed = 50;
bool motor_28byj48_is_homed = false;
int target_28byj48_motor_position = 0;

// Variables for auto-test feedback
bool waiting_for_pin_test_feedback = false;
bool pin_test_confirmed = false;
int test_pin_combination[4] = {0, 0, 0, 0};
unsigned long test_feedback_timeout = 0;

void setup28BYJ48Motor() {
    pinMode(motor_28byj48_pin_1, OUTPUT);
    pinMode(motor_28byj48_pin_2, OUTPUT);
    pinMode(motor_28byj48_pin_3, OUTPUT);
    pinMode(motor_28byj48_pin_4, OUTPUT);
    
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
    
    Serial.printf("28BYJ-48 Stepper Motor initialized - Pins: %d, %d, %d, %d\n", 
                  motor_28byj48_pin_1, motor_28byj48_pin_2, motor_28byj48_pin_3, motor_28byj48_pin_4);
}

void setup28BYJ48MotorWithPins(int pin1, int pin2, int pin3, int pin4) {
    motor_28byj48_pin_1 = pin1;
    motor_28byj48_pin_2 = pin2;
    motor_28byj48_pin_3 = pin3;
    motor_28byj48_pin_4 = pin4;
    
    setup28BYJ48Motor();
}

void set28BYJ48PinConfiguration(int pin1, int pin2, int pin3, int pin4) {
    // Disable old pins
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
    
    // Set new pins and save to NVS
    set28BYJ48Pins(pin1, pin2, pin3, pin4);
    
    // Re-initialize hardware
    pinMode(motor_28byj48_pin_1, OUTPUT);
    pinMode(motor_28byj48_pin_2, OUTPUT);
    pinMode(motor_28byj48_pin_3, OUTPUT);
    pinMode(motor_28byj48_pin_4, OUTPUT);
    
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
    
    Serial.printf("28BYJ-48 pin configuration changed and saved to NVS - New pins: %d, %d, %d, %d\n", 
                  pin1, pin2, pin3, pin4);
}

void get28BYJ48PinConfiguration(int* pin1, int* pin2, int* pin3, int* pin4) {
    *pin1 = motor_28byj48_pin_1;
    *pin2 = motor_28byj48_pin_2;
    *pin3 = motor_28byj48_pin_3;
    *pin4 = motor_28byj48_pin_4;
}

void set28BYJ48MotorPins(int step) {
    digitalWrite(motor_28byj48_pin_1, motor_28byj48_sequence[step][0]);
    digitalWrite(motor_28byj48_pin_2, motor_28byj48_sequence[step][1]);
    digitalWrite(motor_28byj48_pin_3, motor_28byj48_sequence[step][2]);
    digitalWrite(motor_28byj48_pin_4, motor_28byj48_sequence[step][3]);
    current_28byj48_step_index = step;
}

void move28BYJ48Motor(int steps, int direction) {
    for (int i = 0; i < steps; i++) {
        int step_index;
        
        if (direction > 0) {
            step_index = (current_28byj48_step_index + 1) % 8;
        } else {
            step_index = (current_28byj48_step_index - 1 + 8) % 8;
        }
        
        set28BYJ48MotorPins(step_index);
        current_28byj48_motor_position += direction;
        delay(MOTOR_28BYJ48_STEP_DELAY_MS);
    }
    
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
}

void move28BYJ48MotorWithSpeed(int steps, int direction, int speed) {
    speed = constrain(speed, 0, 90);
    int delayMs;
    
    if (speed < 30) {
        delayMs = map(speed, 0, 29, 50, 20);
        
        for (int i = 0; i < steps; i++) {
            if (direction > 0) {
                current_28byj48_step_index = (current_28byj48_step_index + 1) % 8;
            } else {
                current_28byj48_step_index = (current_28byj48_step_index - 1 + 8) % 8;
            }
            
            digitalWrite(motor_28byj48_pin_1, motor_28byj48_sequence[current_28byj48_step_index][0]);
            digitalWrite(motor_28byj48_pin_2, motor_28byj48_sequence[current_28byj48_step_index][1]);
            digitalWrite(motor_28byj48_pin_3, motor_28byj48_sequence[current_28byj48_step_index][2]);
            digitalWrite(motor_28byj48_pin_4, motor_28byj48_sequence[current_28byj48_step_index][3]);
            
            current_28byj48_motor_position += direction;  // direction is now +1 or -1
            delay(delayMs);
        }
    } 
    else if (speed < 70) {
        delayMs = map(speed, 30, 69, 20, 3);
        
        for (int i = 0; i < steps; i++) {
            if (direction > 0) {
                current_28byj48_step_index = (current_28byj48_step_index + 1) % 8;
            } else {
                current_28byj48_step_index = (current_28byj48_step_index - 1 + 8) % 8;
            }
            
            digitalWrite(motor_28byj48_pin_1, motor_28byj48_sequence[current_28byj48_step_index][0]);
            digitalWrite(motor_28byj48_pin_2, motor_28byj48_sequence[current_28byj48_step_index][1]);
            digitalWrite(motor_28byj48_pin_3, motor_28byj48_sequence[current_28byj48_step_index][2]);
            digitalWrite(motor_28byj48_pin_4, motor_28byj48_sequence[current_28byj48_step_index][3]);
            
            current_28byj48_motor_position += direction;  // direction is now +1 or -1
            delay(delayMs);
        }
    }
    else {
        int delayMicros = map(speed, 70, 90, 3000, 500);
        
        for (int i = 0; i < steps; i++) {
            if (direction > 0) {
                current_28byj48_step_index = (current_28byj48_step_index + 1) % 8;
            } else {
                current_28byj48_step_index = (current_28byj48_step_index - 1 + 8) % 8;
            }
            
            digitalWrite(motor_28byj48_pin_1, motor_28byj48_sequence[current_28byj48_step_index][0]);
            digitalWrite(motor_28byj48_pin_2, motor_28byj48_sequence[current_28byj48_step_index][1]);
            digitalWrite(motor_28byj48_pin_3, motor_28byj48_sequence[current_28byj48_step_index][2]);
            digitalWrite(motor_28byj48_pin_4, motor_28byj48_sequence[current_28byj48_step_index][3]);
            
            current_28byj48_motor_position += direction;  // direction is now +1 or -1
            delayMicroseconds(delayMicros);
        }
    }
    
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
}

void move28BYJ48MotorToPosition(int position) {
    target_28byj48_motor_position = position;
    int steps_to_move = position - current_28byj48_motor_position;
    int direction = (steps_to_move > 0) ? 1 : -1;
    
    motor_28byj48_is_moving = true;
    move28BYJ48Motor(abs(steps_to_move), direction);
    motor_28byj48_is_moving = false;
}

void stop28BYJ48Motor() {
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
    
    motor_28byj48_is_moving = false;
    target_28byj48_motor_position = current_28byj48_motor_position;
    
    Serial.println("28BYJ-48 Motor stopped");
}

void test28BYJ48PinCombination(int pin1, int pin2, int pin3, int pin4, int testSteps, int delayMs) {
    Serial.printf("\n=== Test pin combination: %d, %d, %d, %d ===\n", pin1, pin2, pin3, pin4);
    
    digitalWrite(motor_28byj48_pin_1, LOW);
    digitalWrite(motor_28byj48_pin_2, LOW);
    digitalWrite(motor_28byj48_pin_3, LOW);
    digitalWrite(motor_28byj48_pin_4, LOW);
    
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    pinMode(pin3, OUTPUT);
    pinMode(pin4, OUTPUT);
    
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
    digitalWrite(pin3, LOW);
    digitalWrite(pin4, LOW);
    
    int test_step_index = 0;
    
    Serial.printf("Starting test with %d steps, %d ms delay\n", testSteps, delayMs);
    
    for (int i = 0; i < testSteps; i++) {
        test_step_index = (test_step_index + 1) % 8;
        
        digitalWrite(pin1, motor_28byj48_sequence[test_step_index][0]);
        digitalWrite(pin2, motor_28byj48_sequence[test_step_index][1]);
        digitalWrite(pin3, motor_28byj48_sequence[test_step_index][2]);
        digitalWrite(pin4, motor_28byj48_sequence[test_step_index][3]);
        
        delay(delayMs);
    }
    
    digitalWrite(pin1, LOW);
    digitalWrite(pin2, LOW);
    digitalWrite(pin3, LOW);
    digitalWrite(pin4, LOW);
    
    Serial.println("Test complete\n");
}

void autoTest28BYJ48PinCombinations(int basePins[], int pinCount, int testSteps, int delayMs, int betweenDelayMs, int timeoutSeconds) {
    Serial.println("\n========================================");
    Serial.println("=== AUTO-TEST: All pin combinations ===");
    Serial.println("========================================\n");
    Serial.printf("Pin count: %d\n", pinCount);
    Serial.printf("Test steps: %d\n", testSteps);
    Serial.printf("Step delay: %d ms\n", delayMs);
    Serial.printf("Pause between tests: %d ms\n", betweenDelayMs);
    Serial.printf("Timeout per test: %d seconds\n\n", timeoutSeconds);
    
    // Test all possible permutations of 4 pins from the list
    for (int i1 = 0; i1 < pinCount; i1++) {
        for (int i2 = 0; i2 < pinCount; i2++) {
            if (i2 == i1) continue; // Don't reuse the same pin
            
            for (int i3 = 0; i3 < pinCount; i3++) {
                if (i3 == i1 || i3 == i2) continue;
                
                for (int i4 = 0; i4 < pinCount; i4++) {
                    if (i4 == i1 || i4 == i2 || i4 == i3) continue;
                    
                    int pin1 = basePins[i1];
                    int pin2 = basePins[i2];
                    int pin3 = basePins[i3];
                    int pin4 = basePins[i4];
                    
                    Serial.println("\n----------------------------------------");
                    Serial.printf(">>> TESTING COMBINATION: %d, %d, %d, %d <<<\n", pin1, pin2, pin3, pin4);
                    Serial.println("----------------------------------------");
                    
                    // Run test
                    test28BYJ48PinCombination(pin1, pin2, pin3, pin4, testSteps, delayMs);
                    
                    // Wait for user feedback via web server
                    Serial.println("\n>>> WAITING FOR CONFIRMATION <<<");
                    Serial.println("Web server: Did the motor move correctly?");
                    Serial.println("- YES: These pins will be saved");
                    Serial.println("- NO: Next combination will be tested");
                    Serial.printf("Timeout: %d seconds\n\n", timeoutSeconds);
                    
                    // Mark that we are waiting for feedback
                    extern bool waiting_for_pin_test_feedback;
                    extern int test_pin_combination[4];
                    extern unsigned long test_feedback_timeout;
                    
                    waiting_for_pin_test_feedback = true;
                    test_pin_combination[0] = pin1;
                    test_pin_combination[1] = pin2;
                    test_pin_combination[2] = pin3;
                    test_pin_combination[3] = pin4;
                    test_feedback_timeout = millis() + (timeoutSeconds * 1000); // Configurable timeout
                    
                    // Wait for feedback or timeout
                    while (waiting_for_pin_test_feedback && millis() < test_feedback_timeout) {
                        delay(100);
                    }
                    
                    // Check if confirmed
                    extern bool pin_test_confirmed;
                    if (pin_test_confirmed) {
                        Serial.println("\n✓ COMBINATION CONFIRMED!");
                        Serial.printf("✓ Correct pins: %d, %d, %d, %d\n", pin1, pin2, pin3, pin4);
                        Serial.println("✓ Pin configuration applied\n");
                        
                        set28BYJ48PinConfiguration(pin1, pin2, pin3, pin4);
                        
                        Serial.println("========================================");
                        Serial.println("=== AUTO-TEST COMPLETE (SUCCESS) ===");
                        Serial.println("========================================\n");
                        return; // Success - end test
                    }
                    
                    if (!waiting_for_pin_test_feedback) {
                        Serial.println("✗ Combination rejected - testing next combination\n");
                    } else {
                        Serial.println("⏱ Timeout - testing next combination\n");
                    }
                    
                    delay(betweenDelayMs); // Pause between tests
                }
            }
        }
    }
    
    Serial.println("\n========================================");
    Serial.println("=== AUTO-TEST COMPLETE ===");
    Serial.println("!!! NO CORRECT COMBINATION FOUND !!!");
    Serial.println("========================================\n");
}

void home28BYJ48Motor() {
    Serial.println("28BYJ-48 Motor moving to home position...");
    move28BYJ48MotorToPosition(0);
    current_28byj48_motor_position = 0;
    target_28byj48_motor_position = 0;
    motor_28byj48_is_homed = true;
    Serial.println("28BYJ-48 Motor is at home position");
}

int get28BYJ48CurrentMotorPosition() {
    return current_28byj48_motor_position;
}

bool is28BYJ48MotorMoving() {
    return motor_28byj48_is_moving;
}

void set28BYJ48MotorSpeed(int speed) {
    current_28byj48_motor_speed = constrain(speed, 0, 100);
    Serial.printf("28BYJ-48 Motor speed set to %d%%\n", current_28byj48_motor_speed);
}

void move28BYJ48MotorDegrees(float degrees, int direction) {
    int steps = (int)((degrees / 360.0) * MOTOR_28BYJ48_STEPS_PER_REVOLUTION);
    
    Serial.printf("Moving 28BYJ-48 Motor by %.1f degrees (%d steps)\n", degrees, steps);
    
    motor_28byj48_is_moving = true;
    move28BYJ48MotorWithSpeed(steps, direction, current_28byj48_motor_speed);
    motor_28byj48_is_moving = false;
}

void calibrate28BYJ48Motor() {
    Serial.println("28BYJ-48 Motor calibrating...");
    current_28byj48_motor_position = 0;
    target_28byj48_motor_position = 0;
    current_28byj48_step_index = 0;
    motor_28byj48_is_homed = true;
    Serial.println("28BYJ-48 Motor calibrated - current position is now 0");
}

void move28BYJ48MotorWithAcceleration(int steps, int direction, int startSpeed, int endSpeed) {
    motor_28byj48_is_moving = true;
    
    startSpeed = constrain(startSpeed, 1, 100);
    endSpeed = constrain(endSpeed, 1, 100);
    
    Serial.printf("28BYJ-48 Motor moving %d steps with acceleration from %d%% to %d%%\n", 
                  steps, startSpeed, endSpeed);
    
    for (int i = 0; i < steps; i++) {
        int currentSpeed = startSpeed + ((endSpeed - startSpeed) * i) / steps;
        
        if (direction > 0) {
            current_28byj48_step_index = (current_28byj48_step_index + 1) % 8;
        } else {
            current_28byj48_step_index = (current_28byj48_step_index - 1 + 8) % 8;
        }
        
        set28BYJ48MotorPins(current_28byj48_step_index);
        current_28byj48_motor_position += direction;
        
        int delayMs = map(currentSpeed, 1, 100, 20, 1);
        delay(delayMs);
    }
    
    stop28BYJ48Motor();
}

void move28BYJ48MotorSmoothly(int steps, int direction, int speed) {
    motor_28byj48_is_moving = true;
    
    speed = constrain(speed, 1, 100);
    int accelSteps = steps / 4;
    
    Serial.printf("28BYJ-48 Motor moving %d steps smoothly with target speed %d%%\n", steps, speed);
    
    for (int i = 0; i < steps; i++) {
        int currentSpeed;
        
        if (i < accelSteps) {
            currentSpeed = map(i, 0, accelSteps, 10, speed);
        } else if (i >= steps - accelSteps) {
            currentSpeed = map(i, steps - accelSteps, steps, speed, 10);
        } else {
            currentSpeed = speed;
        }
        
        if (direction > 0) {
            current_28byj48_step_index = (current_28byj48_step_index + 1) % 8;
        } else {
            current_28byj48_step_index = (current_28byj48_step_index - 1 + 8) % 8;
        }
        
        set28BYJ48MotorPins(current_28byj48_step_index);
        current_28byj48_motor_position += direction;
        
        int delayMs = map(currentSpeed, 1, 100, 20, 1);
        delay(delayMs);
    }
    
    stop28BYJ48Motor();
}

Motor28BYJ48Status get28BYJ48MotorStatus() {
    Motor28BYJ48Status status;
    status.currentPosition = current_28byj48_motor_position;
    status.targetPosition = target_28byj48_motor_position;
    status.isMoving = motor_28byj48_is_moving;
    status.currentSpeed = current_28byj48_motor_speed;
    status.isHomed = motor_28byj48_is_homed;
    
    return status;
}
