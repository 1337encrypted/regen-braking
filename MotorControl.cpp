#include "MotorControl.h"

MotorControl::MotorControl(
    gpio_num_t relayPin,
    gpio_num_t rpwmPin,
    gpio_num_t lpwmPin,
    CurrentSensor* currentSensor
)
    : m_relayPin(relayPin)
    , m_rpwmPin(rpwmPin)
    , m_lpwmPin(lpwmPin)
    , m_state(State::STOPPED)
    , m_mutex(NULL)
    , m_motorStartTime(0)
    , m_timerActive(false)
    , m_currentSpeed(128)  // Default to 50% speed
    , m_currentSensor(currentSensor)
{
}

MotorControl::~MotorControl() {
    if (m_mutex != NULL) {
        vSemaphoreDelete(m_mutex);
        m_mutex = NULL;
    }
}

bool MotorControl::begin() {
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == NULL) {
        Serial.println("ERROR: Failed to create MotorControl mutex");
        return false;
    }

    // Configure relay pin
    pinMode(m_relayPin, OUTPUT);
    digitalWrite(m_relayPin, LOW);  // Default: charging mode

    // Configure PWM for BTS7960 (ESP32 Core 3.x API)
    ledcAttach(m_rpwmPin, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(m_rpwmPin, 0);  // Start with motor off

    ledcAttach(m_lpwmPin, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(m_lpwmPin, 0);  // Always 0 for forward-only operation

    Serial.println("\n--- Motor Control Initialized ---");
    Serial.printf("  Relay: GPIO %d (LOW=charging, HIGH=motor)\n", m_relayPin);
    Serial.printf("  RPWM: GPIO %d (forward PWM)\n", m_rpwmPin);
    Serial.printf("  LPWM: GPIO %d (always LOW)\n", m_lpwmPin);
    Serial.printf("  Auto-stop timeout: %lu seconds\n", MOTOR_TIMEOUT_MS / 1000);
    Serial.printf("  Overcurrent threshold: %.0f mA (%.1f A)\n",
                  OVERCURRENT_THRESHOLD_MA, OVERCURRENT_THRESHOLD_MA / 1000.0f);
    Serial.println("  Initial state: CHARGING");

    m_state = State::CHARGING;
    return true;
}

void MotorControl::setRelay(bool on) {
    digitalWrite(m_relayPin, on ? HIGH : LOW);
    Serial.printf("Relay: %s\n", on ? "ON (Motor mode)" : "OFF (Charging mode)");
}

void MotorControl::setPWM(uint8_t speed) {
    ledcWrite(m_rpwmPin, speed);  // ESP32 Core 3.x uses pin instead of channel
    ledcWrite(m_lpwmPin, 0);      // Always keep reverse OFF
    Serial.printf("PWM: RPWM=%d, LPWM=0\n", speed);
}

void MotorControl::stopMotorImmediate() {
    setPWM(0);           // Stop PWM first
    delay(100);          // Wait for motor to stop spinning
    setRelay(false);     // Switch relay to charging mode
}

bool MotorControl::startMotor() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Serial.println("\n=== Starting Motor ===");

        // Safety: ensure we're not already running
        if (m_state == State::RUNNING) {
            Serial.println("Motor already running");
            xSemaphoreGive(m_mutex);
            return true;
        }

        // Sequence: Relay ON first, then PWM
        setRelay(true);       // Switch to motor mode
        delay(50);            // Small delay for relay to settle
        setPWM(m_currentSpeed);  // Use current speed setting

        // Start safety timer
        m_motorStartTime = millis();
        m_timerActive = true;
        m_state = State::RUNNING;

        Serial.printf("Motor started at speed %d (%.0f%%)\n", m_currentSpeed, (m_currentSpeed / 255.0f) * 100.0f);
        Serial.printf("Auto-stop in %lu seconds\n", MOTOR_TIMEOUT_MS / 1000);
        Serial.println("======================\n");

        xSemaphoreGive(m_mutex);
        return true;
    }

    Serial.println("ERROR: Failed to acquire mutex for startMotor");
    return false;
}

bool MotorControl::startCharging() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Serial.println("\n=== Starting Charging ===");

        // Safety: ensure we're not already charging
        if (m_state == State::CHARGING) {
            Serial.println("Already in charging mode");
            xSemaphoreGive(m_mutex);
            return true;
        }

        // Stop timer
        m_timerActive = false;

        // Sequence: Stop PWM first, then switch relay
        stopMotorImmediate();
        m_state = State::CHARGING;

        Serial.println("Charging mode activated");
        Serial.println("Motor disconnected from driver");
        Serial.println("Motor connected to buck-boost converter");
        Serial.println("=========================\n");

        xSemaphoreGive(m_mutex);
        return true;
    }

    Serial.println("ERROR: Failed to acquire mutex for startCharging");
    return false;
}

void MotorControl::emergencyStop() {
    Serial.println("\n!!! EMERGENCY STOP !!!");

    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        m_timerActive = false;
        stopMotorImmediate();
        m_state = State::STOPPED;
        xSemaphoreGive(m_mutex);
    } else {
        // Force stop even without mutex in emergency
        stopMotorImmediate();
        m_state = State::STOPPED;
    }

    Serial.println("System halted\n");
}

bool MotorControl::update() {
    // Check if motor is running
    if (!m_timerActive || m_state != State::RUNNING) {
        return false;
    }

    bool stateChanged = false;

    // Check overcurrent protection
    if (m_currentSensor != nullptr) {
        if (m_currentSensor->isOverCurrent(OVERCURRENT_THRESHOLD_MA)) {
            Serial.println("\n!!! OVERCURRENT DETECTED !!!");
            Serial.printf("Threshold: %.0f mA\n", OVERCURRENT_THRESHOLD_MA);
            emergencyStop();
            return true;
        }
    }

    // Check timeout
    unsigned long elapsed = millis() - m_motorStartTime;
    if (elapsed >= MOTOR_TIMEOUT_MS) {
        Serial.println("\n=== Motor Timeout ===");
        Serial.printf("Auto-stop after %lu seconds\n", MOTOR_TIMEOUT_MS / 1000);
        startCharging();
        stateChanged = true;
    }

    return stateChanged;
}

String MotorControl::getStateString() const {
    switch (m_state) {
        case State::STOPPED:  return "stopped";
        case State::RUNNING:  return "running";
        case State::CHARGING: return "charging";
        default:              return "unknown";
    }
}

unsigned long MotorControl::getTimeRemaining() const {
    if (!m_timerActive || m_state != State::RUNNING) {
        return 0;
    }

    unsigned long elapsed = millis() - m_motorStartTime;
    if (elapsed >= MOTOR_TIMEOUT_MS) {
        return 0;
    }

    return (MOTOR_TIMEOUT_MS - elapsed) / 1000;  // Return seconds
}

bool MotorControl::setSpeed(uint8_t speed) {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        m_currentSpeed = speed;

        // If motor is running, update PWM immediately
        if (m_state == State::RUNNING) {
            setPWM(m_currentSpeed);
            Serial.printf("Speed changed to %d (%.0f%%)\n", m_currentSpeed, (m_currentSpeed / 255.0f) * 100.0f);
        }

        xSemaphoreGive(m_mutex);
        return true;
    }

    return false;
}
