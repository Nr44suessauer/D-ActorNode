#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

// Motor-Pin-Definitionen
#define MOTOR_PIN_1 19    // IN1
#define MOTOR_PIN_2 20    // IN2
#define MOTOR_PIN_3 21    // IN3
#define MOTOR_PIN_4 47    // IN4

// Button-Pin-Definition
#define BUTTON_PIN 45     // Button Input Pin

// Optimierte Werte für den 28BYJ-48 Motor - schnellere Geschwindigkeit
#define STEP_DELAY_MS 1     // Reduziert auf 1ms für höhere Grundgeschwindigkeit
#define STEPS_PER_REVOLUTION 4096  // Der 28BYJ-48 benötigt 4096 Schritte für eine volle Umdrehung
#define MAX_SPEED_DELAY 0.5   // Minimale Verzögerung für höchste Geschwindigkeit
#define MIN_SPEED_DELAY 10    // Reduziert für insgesamt schnellere Bewegung

// Funktionsprototypen
void setupMotor();
void setMotorPins(int step);
void moveMotor(int steps, int direction);
void moveMotorToPosition(int position);
void moveMotorWithSpeed(int steps, int direction, int speed);

// Erweiterte Motor-Funktionen
void stopMotor();
void homeMotor();
int getCurrentMotorPosition();
bool isMotorMoving();
void setMotorSpeed(int speed);
void moveMotorDegrees(float degrees, int direction);
void calibrateMotor();

// Geschwindigkeitsprofile
void moveMotorWithAcceleration(int steps, int direction, int startSpeed, int endSpeed);
void moveMotorSmoothly(int steps, int direction, int speed);

// Motor-Status-Funktionen
typedef struct {
    int currentPosition;
    int targetPosition;
    bool isMoving;
    int currentSpeed;
    bool isHomed;
} MotorStatus;

MotorStatus getMotorStatus();

// Button Funktionsprototypen
void setupButton();
bool getButtonState();

#endif // MOTOR_H