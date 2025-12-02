#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "CurrentSensor.h"

/**
 * @brief Motor Control Class for BTS7960 + Relay
 *
 * Controls motor through BTS7960 motor driver and relay switching
 * - Relay OFF: Motor → Buck-boost converter (CHARGING mode)
 * - Relay ON: Motor → BTS7960 driver (MOTOR mode)
 * - BTS7960: RPWM (GPIO19), LPWM (GPIO18, always LOW for forward only)
 * - Safety: 30-second auto-stop timer
 * - Overcurrent protection: 40A threshold
 */
class MotorControl {
public:
    enum class State {
        STOPPED,    // Motor off, relay off, in transition
        RUNNING,    // Motor running via BTS7960
        CHARGING    // Motor generating, charging battery via buck-boost
    };

private:
    // GPIO pins
    gpio_num_t m_relayPin;      // GPIO15
    gpio_num_t m_rpwmPin;       // GPIO19 (forward PWM)
    gpio_num_t m_lpwmPin;       // GPIO18 (reverse PWM, always 0)

    // PWM configuration
    static constexpr int PWM_CHANNEL_R = 0;
    static constexpr int PWM_CHANNEL_L = 1;
    static constexpr int PWM_FREQ = 1000;      // 1 kHz
    static constexpr int PWM_RESOLUTION = 8;   // 8-bit (0-255)
    static constexpr int PWM_FULL_SPEED = 255;

    // Safety configuration
    static constexpr unsigned long MOTOR_TIMEOUT_MS = 30000;  // 30 seconds
    static constexpr float OVERCURRENT_THRESHOLD_MA = 60000.0f;  // 60A = 60000mA (temporary for testing)

    // State management
    State m_state;
    SemaphoreHandle_t m_mutex;
    unsigned long m_motorStartTime;
    bool m_timerActive;

    // Speed control
    uint8_t m_currentSpeed;

    // Current sensor reference for overcurrent protection
    CurrentSensor* m_currentSensor;

    // Internal control methods
    void setRelay(bool on);
    void setPWM(uint8_t speed);
    void stopMotorImmediate();

public:
    /**
     * @brief Construct MotorControl
     * @param relayPin GPIO for relay control (GPIO15)
     * @param rpwmPin GPIO for BTS7960 RPWM (GPIO19)
     * @param lpwmPin GPIO for BTS7960 LPWM (GPIO18)
     * @param currentSensor Pointer to current sensor for overcurrent protection
     */
    MotorControl(
        gpio_num_t relayPin,
        gpio_num_t rpwmPin,
        gpio_num_t lpwmPin,
        CurrentSensor* currentSensor
    );

    ~MotorControl();

    /**
     * @brief Initialize motor control
     * @return true if successful, false otherwise
     */
    bool begin();

    /**
     * @brief Start motor (relay ON, PWM active)
     * @return true if successful, false otherwise
     */
    bool startMotor();

    /**
     * @brief Stop motor and enable charging (relay OFF, PWM off)
     * @return true if successful, false otherwise
     */
    bool startCharging();

    /**
     * @brief Emergency stop (stop everything immediately)
     */
    void emergencyStop();

    /**
     * @brief Update function - call in loop() to check safety timers
     * @return true if state changed (timer expired or overcurrent), false otherwise
     */
    bool update();

    /**
     * @brief Get current state
     * @return Current state
     */
    State getState() const { return m_state; }

    /**
     * @brief Get state as string
     * @return State name
     */
    String getStateString() const;

    /**
     * @brief Get time remaining before auto-stop (if motor running)
     * @return Seconds remaining, or 0 if not running
     */
    unsigned long getTimeRemaining() const;

    /**
     * @brief Check if motor is running
     * @return true if motor is running
     */
    bool isRunning() const { return m_state == State::RUNNING; }

    /**
     * @brief Check if charging
     * @return true if charging
     */
    bool isCharging() const { return m_state == State::CHARGING; }

    /**
     * @brief Set motor speed (0-255)
     * @param speed PWM value (0=stopped, 255=full speed)
     * @return true if successful (only works when motor is running)
     */
    bool setSpeed(uint8_t speed);

    /**
     * @brief Get current motor speed
     * @return Current PWM speed (0-255)
     */
    uint8_t getSpeed() const { return m_currentSpeed; }
};
