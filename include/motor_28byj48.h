#ifndef MOTOR_28BYJ48_H
#define MOTOR_28BYJ48_H

#include <Arduino.h>

// Motor pin definitions for 28BYJ-48 (default values)
#define MOTOR_28BYJ48_DEFAULT_PIN_1 6     // GPIO6 = IN1
#define MOTOR_28BYJ48_DEFAULT_PIN_2 4     // GPIO4 = IN2
#define MOTOR_28BYJ48_DEFAULT_PIN_3 7     // GPIO7 = IN3
#define MOTOR_28BYJ48_DEFAULT_PIN_4 5     // GPIO5 = IN4

// Global pin variables (can be changed at runtime)
extern int motor_28byj48_pin_1;
extern int motor_28byj48_pin_2;
extern int motor_28byj48_pin_3;
extern int motor_28byj48_pin_4;

// Optimized values for the 28BYJ-48 motor
#define MOTOR_28BYJ48_STEP_DELAY_MS 1     // Reduced to 1ms for higher base speed
#define MOTOR_28BYJ48_STEPS_PER_REVOLUTION 4096  // 28BYJ-48 requires 4096 steps for one full revolution
#define MOTOR_28BYJ48_MAX_SPEED_DELAY 0.5   // Minimum delay for highest speed
#define MOTOR_28BYJ48_MIN_SPEED_DELAY 10    // Reduced for overall faster movement

// Funktionsprototypen
void setup28BYJ48Motor();
void setup28BYJ48MotorWithPins(int pin1, int pin2, int pin3, int pin4); // Setup mit benutzerdefinierten Pins
void set28BYJ48PinConfiguration(int pin1, int pin2, int pin3, int pin4); // Change pin configuration at runtime
void get28BYJ48PinConfiguration(int* pin1, int* pin2, int* pin3, int* pin4); // Returns current pin configuration
void set28BYJ48MotorPins(int step);
void move28BYJ48Motor(int steps, int direction);
void move28BYJ48MotorToPosition(int position);
void move28BYJ48MotorWithSpeed(int steps, int direction, int speed);
void test28BYJ48PinCombination(int pin1, int pin2, int pin3, int pin4, int testSteps, int delayMs); // Testet eine Pin-Kombination
void autoTest28BYJ48PinCombinations(int basePins[], int pinCount, int testSteps, int delayMs, int betweenDelayMs, int timeoutSeconds = 10); // Testet alle Pin-Kombinationen automatisch
// Funktionsprototypen
void setup28BYJ48Motor();
void set28BYJ48MotorPins(int step);
void move28BYJ48Motor(int steps, int direction);
void move28BYJ48MotorToPosition(int position);
void move28BYJ48MotorWithSpeed(int steps, int direction, int speed);

// Erweiterte Motor-Funktionen
void stop28BYJ48Motor();
void home28BYJ48Motor();
int get28BYJ48CurrentMotorPosition();
bool is28BYJ48MotorMoving();
void set28BYJ48MotorSpeed(int speed);
void move28BYJ48MotorDegrees(float degrees, int direction);
void calibrate28BYJ48Motor();

// Geschwindigkeitsprofile
void move28BYJ48MotorWithAcceleration(int steps, int direction, int startSpeed, int endSpeed);
void move28BYJ48MotorSmoothly(int steps, int direction, int speed);

// Motor-Status-Funktionen
typedef struct {
    int currentPosition;
    int targetPosition;
    bool isMoving;
    int currentSpeed;
    bool isHomed;
} Motor28BYJ48Status;

Motor28BYJ48Status get28BYJ48MotorStatus();

#endif // MOTOR_28BYJ48_H
