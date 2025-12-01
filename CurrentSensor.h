#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Current Sensor Class for ACS758LCB-50
 *
 * Bidirectional ±50A Hall-effect current sensor
 * - Supply Voltage: 3.3V
 * - Zero Current Output: 1.65V (Vcc/2)
 * - Sensitivity: 40mV/A
 * - Output Range: 0.65V to 2.65V (-25A to +25A typical)
 */
class CurrentSensor {
private:
    gpio_num_t m_pin;
    SemaphoreHandle_t m_mutex;

    // Calibration constants
    static constexpr float ZERO_CURRENT_VOLTAGE = 1.65f;  // V (at 0A with 3.3V supply)
    static constexpr float SENSITIVITY = 0.040f;          // V/A (40mV/A)
    static constexpr float ADC_VREF = 3.3f;               // V
    static constexpr int ADC_RESOLUTION = 4095;           // 12-bit ADC

    // Filtering
    static constexpr int SAMPLE_COUNT = 10;

    // Convert ADC reading to voltage
    float adcToVoltage(int adcValue) const;

    // Convert voltage to current (mA)
    float voltageToCurrent(float voltage) const;

public:
    /**
     * @brief Construct CurrentSensor
     * @param pin GPIO pin connected to ACS758 output (must be ADC-capable)
     */
    CurrentSensor(gpio_num_t pin);

    ~CurrentSensor();

    /**
     * @brief Initialize the sensor
     * @return true if successful, false otherwise
     */
    bool begin();

    /**
     * @brief Read current in milliamps
     * @param averaged If true, returns averaged reading over multiple samples
     * @return Current in mA (positive = one direction, negative = opposite)
     */
    float readCurrentMilliAmps(bool averaged = true);

    /**
     * @brief Read raw ADC value
     * @return ADC value (0-4095)
     */
    int readRawADC();

    /**
     * @brief Check if current exceeds threshold
     * @param thresholdMilliAmps Threshold in mA (absolute value)
     * @return true if |current| > threshold
     */
    bool isOverCurrent(float thresholdMilliAmps);
};
