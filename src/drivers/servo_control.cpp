#include "servo_control.h"

// Legacy alias points to servo #1 pin for backward compatibility.
int SERVO_GPIO_PIN = 14;

// Default pins for up to 3 servos.
int SERVO_GPIO_PINS[MAX_SERVOS] = {14, 15, 16};

// Track current servo positions.
static int currentServoAngles[MAX_SERVOS] = {90, 90, 90};

static bool isValidServoId(uint8_t servoId) {
    return servoId >= 1 && servoId <= MAX_SERVOS;
}

static ledc_channel_t channelForServo(uint8_t servoId) {
    switch (servoId) {
        case 1: return LEDC_CHANNEL_0;
        case 2: return LEDC_CHANNEL_1;
        case 3: return LEDC_CHANNEL_2;
        default: return LEDC_CHANNEL_0;
    }
}

static void configureServoChannel(uint8_t servoId) {
    if (!isValidServoId(servoId)) {
        return;
    }

    const int index = servoId - 1;
    ledc_channel_config_t ledc_channel = {
        .gpio_num = SERVO_GPIO_PINS[index],
        .speed_mode = LEDC_MODE,
        .channel = channelForServo(servoId),
        .timer_sel = LEDC_TIMER,
        .duty = calculate_duty_us(SERVO_NEUTRAL, LEDC_TIMER_13_BIT),
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

uint32_t calculate_duty_us(uint32_t pulse_width_us, uint32_t timer_resolution_bits) {
    uint32_t max_duty = (1 << timer_resolution_bits) - 1;
    return (pulse_width_us * max_duty) / (1000000 / PWM_FREQUENCY);
}

void setupServo() {
    // Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT, // 13-bit resolution for high precision
        .timer_num = LEDC_TIMER,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    // Keep legacy alias in sync with servo #1 pin.
    SERVO_GPIO_PIN = SERVO_GPIO_PINS[0];

    for (uint8_t servoId = 1; servoId <= MAX_SERVOS; servoId++) {
        configureServoChannel(servoId);
        setServoAngleById(servoId, 90);
    }

    Serial.printf("Servos initialisiert: S1@IO%d, S2@IO%d, S3@IO%d\n",
                  SERVO_GPIO_PINS[0], SERVO_GPIO_PINS[1], SERVO_GPIO_PINS[2]);
}

void setServoAngle(int angle) {
    // Legacy behavior: target servo #1
    setServoAngleById(1, angle);
}

bool setServoAngleById(uint8_t servoId, int angle) {
    if (!isValidServoId(servoId)) {
        return false;
    }

    const int index = servoId - 1;

    // Constrain the angle to valid range
    angle = constrain(angle, 0, SERVO_MAX_DEGREE);

    // Map the angle to pulse width
    uint32_t pulse_width = map(angle, 0, SERVO_MAX_DEGREE, SERVO_MIN_PULSE, SERVO_MAX_PULSE);

    // Calculate duty cycle
    uint32_t duty = calculate_duty_us(pulse_width, LEDC_TIMER_13_BIT);

    // Set duty cycle on dedicated channel
    ledc_set_duty(LEDC_MODE, channelForServo(servoId), duty);
    ledc_update_duty(LEDC_MODE, channelForServo(servoId));

    // Update current position
    currentServoAngles[index] = angle;

    Serial.print("Servo ");
    Serial.print(servoId);
    Serial.print(" Angle set to: ");
    Serial.println(angle);
    return true;
}

void sweepServo(int targetAngle, int speed) {
    // Constrain target angle
    targetAngle = constrain(targetAngle, 0, SERVO_MAX_DEGREE);
    
    // Get the current position
    int startAngle = currentServoAngles[0];
    
    // Determine direction and step size
    int step = (targetAngle > startAngle) ? 1 : -1;
    
    // Move the servo step by step
    for (int angle = startAngle; angle != targetAngle + step; angle += step) {
        setServoAngle(angle);
        delay(speed); // delay between steps for smooth movement
    }
}

int getCurrentServoAngle() {
    return currentServoAngles[0];
}

int getCurrentServoAngleById(uint8_t servoId) {
    if (!isValidServoId(servoId)) {
        return -1;
    }
    return currentServoAngles[servoId - 1];
}

bool reconfigureServoPin(uint8_t servoId, int pin) {
    if (!isValidServoId(servoId)) {
        return false;
    }

    SERVO_GPIO_PINS[servoId - 1] = pin;
    SERVO_GPIO_PIN = SERVO_GPIO_PINS[0];

    configureServoChannel(servoId);
    setServoAngleById(servoId, getCurrentServoAngleById(servoId));
    return true;
}