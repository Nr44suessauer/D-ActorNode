#ifndef ADVANCED_MOTOR_H
#define ADVANCED_MOTOR_H

#include <Arduino.h>

// Pin definitions for stepper motor
#define STEP_PIN 37
#define DIR_PIN 36
#define ENABLE_PIN -1  // Optional: Pin zum Aktivieren/Deaktivieren des Motors

// Multi-motor support (same concept as servo)
#define MAX_ADVANCED_MOTORS 3

extern int ADV_MOTOR_STEP_PINS[MAX_ADVANCED_MOTORS];
extern int ADV_MOTOR_DIR_PINS[MAX_ADVANCED_MOTORS];
extern int ADV_MOTOR_ENABLE_PINS[MAX_ADVANCED_MOTORS];

// Motor configuration
#define ADVANCED_STEPS_PER_REVOLUTION 200  // Standard for NEMA17 (1.8 deg per step)
#define MICROSTEPS 1                       // Microstepping factor
#define MAX_SPEED_RPM 500                  // Maximum speed in RPM
#define DEFAULT_SPEED_RPM 100              // Default speed in RPM
#define ACCELERATION_STEPS 50    // Steps for acceleration/deceleration

// Debug configuration
#define MOTOR_DEBUG_ENABLED true           // Enable/disable debug output (at compile time)

// Global debug variable (changeable at runtime)
extern bool motorDebugEnabled;

// Debug macros with runtime check
#if MOTOR_DEBUG_ENABLED
  #define MOTOR_DEBUG_PRINT(x) do { if (motorDebugEnabled) Serial.print(x); } while(0)
  #define MOTOR_DEBUG_PRINTLN(x) do { if (motorDebugEnabled) Serial.println(x); } while(0)
  #define MOTOR_DEBUG_PRINTF(format, ...) do { if (motorDebugEnabled) Serial.printf(format, __VA_ARGS__); } while(0)
#else
  #define MOTOR_DEBUG_PRINT(x)
  #define MOTOR_DEBUG_PRINTLN(x)
  #define MOTOR_DEBUG_PRINTF(format, ...)
#endif

// Motor status struct (simplified)
typedef struct {
    int currentPosition;
    int targetPosition;
    int virtualHomePosition;
    bool isMoving;
    int currentSpeed;
    bool isHomed;
    bool isEnabled;
    int targetPassCount;
    int currentPassCount;
    bool isPassingButton;
} AdvancedMotorStatus;

// Class for advanced stepper motor control
class AdvancedStepperMotor {
private:
    int stepPin;
    int dirPin;
    int enablePin;
    
    int currentPosition;
    int targetPosition;
    int virtualHomePosition; // Virtuelle Home-Position
    bool isMoving;
    bool isEnabled;
    bool isHomed;
    
    // Button-Pass Variablen
    int targetPassCount;     // Desired number of button passes
    int currentPassCount;    // Current number of button passes reached
    bool isPassingButton;    // Flag whether button passes are currently being executed

    
    int stepsPerRevolution;
    int currentSpeedRPM;
    unsigned long stepDelayMicros;
    unsigned long lastStepTime;
    
    void calculateStepDelay();
    
public:
    AdvancedStepperMotor(int stepPin, int dirPin, int enablePin = -1, int stepsPerRevolution = 200);
    
    // Initialization
    void begin();
    void reconfigurePins(int newStepPin, int newDirPin, int newEnablePin = -1);
    void enable();
    void disable();
    void setPinsIdle();  // Neue Funktion: Pins in Ruhezustand setzen
    
    // Grundlegende Bewegungsfunktionen
    void setSpeed(int rpm);
    void setDirection(bool clockwise);
    void step();
    void moveSteps(int steps);
    // Basis-Bewegungsfunktionen (nur von UI verwendet)
    void moveTo(int position);
    void moveRelative(int steps);
    
    // Steuerungsfunktionen (nur von UI verwendet)  
    void stop();
    void setHome();
    void homeToButton();  // Neue Funktion: Fahre zur Button-Position als Home
    void passButtonTimes(int count); // Neue Funktion: Passiere Button n-mal
    void setVirtualHome(); // Neue Funktion: Setze aktuelle Position als virtuelle Home
    void moveToVirtualHome(); // Neue Funktion: Fahre zur virtuellen Home-Position

    
    // Status-Funktionen
    int getCurrentPosition();
    int getTargetPosition();
    int getVirtualHomePosition(); // Getter for virtual home position
    bool getIsMoving();
    bool getIsEnabled();
    bool getIsHomed();

    int getCurrentSpeed();
    int getTargetPassCount();
    int getCurrentPassCount();
    bool getIsPassingButton();
    AdvancedMotorStatus getStatus();
    
    // Configuration
    void setStepsPerRevolution(int steps);
    void setMicrostepping(int factor);
    
    // Non-blocking movement (for continuous movements)
    void update();  // Must be called regularly in loop()
    void startNonBlockingMoveTo(int position);
    void startNonBlockingMoveSteps(int steps);
};

// Global instance
extern AdvancedStepperMotor advancedMotor;
extern AdvancedStepperMotor advancedMotor2;
extern AdvancedStepperMotor advancedMotor3;

AdvancedStepperMotor& getAdvancedMotorById(uint8_t motorId);
bool configureAdvancedMotorPinsById(uint8_t motorId, int stepPin, int dirPin, int enablePin);

// Hilfsfunktionen
void setupAdvancedMotor();
void updateMotor();
void homeMotorToButton();  // Home-Fahrt zum Button
void passButtonTimes(int count); // Passiere Button n-mal
void calibrateVirtualHome(); // Setze virtuelle Home-Position
void moveToVirtualHome(); // Fahre zur virtuellen Home-Position

// Debug-Funktionen
void setMotorDebug(bool enabled);     // Debug ein/ausschalten
bool getMotorDebugStatus();          // Debug-Status abfragen

// Web-API Funktionen
void handleAdvancedMotorControl();
void handleAdvancedMotorStatus();
void handleAdvancedMotorStop();
void handleAdvancedMotorHome();
void handleAdvancedMotorJog();
void handleAdvancedMotorCalibrate();


#endif // ADVANCED_MOTOR_H