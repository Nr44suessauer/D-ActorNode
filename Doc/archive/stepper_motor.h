#ifndef STEPPER_MOTOR_H
#define STEPPER_MOTOR_H

#include <Arduino.h>

class StepperMotor {
private:
    int dirPin;
    int stepPin;
    int stepsPerRevolution;
    int currentPosition;
    int targetPosition;
    bool isMoving;
    int stepDelay;

public:
    StepperMotor(int dirPin, int stepPin, int stepsPerRevolution = 200);
    
    void begin();
    void setDirection(bool clockwise);
    void step();
    void moveSteps(int steps);
    void moveTo(int position);
    void moveRelative(int steps);
    void setSpeed(int rpm);
    void stop();
    void home();
    
    int getCurrentPosition();
    int getTargetPosition();
    bool isRunning();
    void setStepDelay(int delayMicros);
};

// Global stepper motor instance (optional)
extern StepperMotor stepperMotor;

#endif // STEPPER_MOTOR_H