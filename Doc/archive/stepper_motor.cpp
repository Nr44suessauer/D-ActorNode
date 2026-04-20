#include "stepper_motor.h"
#include <Arduino.h>

StepperMotor::StepperMotor(int dirPin, int stepPin, int stepsPerRevolution) 
    : dirPin(dirPin), stepPin(stepPin), stepsPerRevolution(stepsPerRevolution), 
      currentPosition(0), targetPosition(0), isMoving(false), stepDelay(1000) {
}

void StepperMotor::begin() {
    pinMode(dirPin, OUTPUT);
    pinMode(stepPin, OUTPUT);
    digitalWrite(dirPin, LOW);
    digitalWrite(stepPin, LOW);
}

void StepperMotor::setDirection(bool clockwise) {
    digitalWrite(dirPin, clockwise ? HIGH : LOW);
}

void StepperMotor::step() {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepDelay);
}

void StepperMotor::moveSteps(int steps) {
    if (steps == 0) return;
    
    setDirection(steps > 0);
    int absSteps = abs(steps);
    
    for (int i = 0; i < absSteps; i++) {
        step();
    }
    
    currentPosition += steps;
}

void StepperMotor::moveTo(int position) {
    int stepsToMove = position - currentPosition;
    moveSteps(stepsToMove);
    targetPosition = position;
}

void StepperMotor::moveRelative(int steps) {
    moveSteps(steps);
    targetPosition = currentPosition;
}

void StepperMotor::setSpeed(int rpm) {
    if (rpm > 0) {
        stepDelay = 60000000L / (stepsPerRevolution * rpm * 2);
    }
}

void StepperMotor::stop() {
    isMoving = false;
    targetPosition = currentPosition;
}

void StepperMotor::home() {
    currentPosition = 0;
    targetPosition = 0;
}

int StepperMotor::getCurrentPosition() {
    return currentPosition;
}

int StepperMotor::getTargetPosition() {
    return targetPosition;
}

bool StepperMotor::isRunning() {
    return isMoving;
}

void StepperMotor::setStepDelay(int delayMicros) {
    stepDelay = delayMicros;
}