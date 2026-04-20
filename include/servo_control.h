/**
 * @file servo_control.h
 * @brief Header file for controlling a servo motor using ESP32 and FreeRTOS.
 *
 * This file contains the definitions and function prototypes for initializing
 * and controlling a servo motor connected to an ESP32 microcontroller.
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

// Multi-servo support
#define MAX_SERVOS 3

// Backward-compatible legacy pin alias (servo 1)
extern int SERVO_GPIO_PIN;
extern int SERVO_GPIO_PINS[MAX_SERVOS];

// Definitions for the servo motor
#define SERVO_MIN_PULSE   500       ///< Minimum pulse width in microseconds (0°)
#define SERVO_MAX_PULSE   2500      ///< Maximum pulse width in microseconds (180°)
#define SERVO_NEUTRAL     1500      ///< Neutral position pulse width in microseconds
#define SERVO_MAX_DEGREE  180       ///< Maximum movement in degrees
#define PWM_FREQUENCY     50        ///< PWM frequency in Hz

// LEDC timer and channel definitions
#define LEDC_TIMER        LEDC_TIMER_0       ///< LEDC timer used for PWM
#define LEDC_MODE         LEDC_LOW_SPEED_MODE///< LEDC mode used for PWM
#define LEDC_CHANNEL      LEDC_CHANNEL_0     ///< Legacy channel alias for servo 1

/**
 * @brief Calculate the duty cycle in microseconds for a given pulse width.
 *
 * This function calculates the duty cycle required to generate a specific pulse width
 * based on the timer resolution.
 *
 * @param pulse_width_us The desired pulse width in microseconds.
 * @param timer_resolution_bits The resolution of the timer in bits.
 * @return The calculated duty cycle in microseconds.
 */
uint32_t calculate_duty_us(uint32_t pulse_width_us, uint32_t timer_resolution_bits);

/**
 * @brief Initialize the servo motor.
 *
 * This function initializes the servo motor by configuring the LEDC timer and channel
 * for PWM generation.
 */
void setupServo();

/**
 * @brief Set the servo motor to a specific angle.
 *
 * This function sets the servo motor to a specified angle by calculating the appropriate
 * pulse width and updating the PWM duty cycle.
 *
 * @param angle The desired angle to set the servo motor to (0° to 180°).
 */
void setServoAngle(int angle);

/**
 * @brief Set a specific servo motor to a specific angle.
 *
 * @param servoId Servo ID (1..MAX_SERVOS)
 * @param angle Desired angle (0..180)
 * @return true if successful, false if servoId is invalid
 */
bool setServoAngleById(uint8_t servoId, int angle);

/**
 * @brief Sweep the servo motor from one position to another.
 *
 * This function smoothly moves the servo from its current position to the target position.
 *
 * @param targetAngle The desired target angle.
 * @param speed Speed of movement (delay in ms between each degree of movement).
 */
void sweepServo(int targetAngle, int speed = 15);

/**
 * @brief Get the current angle of the servo motor.
 *
 * @return The current angle of the servo motor.
 */
int getCurrentServoAngle();

/**
 * @brief Get current angle of one servo.
 *
 * @param servoId Servo ID (1..MAX_SERVOS)
 * @return Current angle, or -1 if invalid servo ID
 */
int getCurrentServoAngleById(uint8_t servoId);

/**
 * @brief Reconfigure one servo pin and reinitialize that servo channel.
 *
 * @param servoId Servo ID (1..MAX_SERVOS)
 * @param pin GPIO pin number
 * @return true if successful, false otherwise
 */
bool reconfigureServoPin(uint8_t servoId, int pin);

#endif // SERVO_CONTROL_H