#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Voltage Sensor Class using Voltage Divider
 *
 * Measures battery voltage through resistor divider
 * - R1: 10kΩ (high side)
 * - R2: 3.3kΩ (low side to GND)
 * - Input Range: 0-14.4V
 * - Output to ADC: 0-3.3V
 */
class VoltageSensor {
private:
    gpio_num_t m_pin;
    SemaphoreHandle_t m_mutex;

    // Voltage divider constants
    static constexpr float R1 = 10000.0f;   // 10kΩ
    static constexpr float R2 = 3300.0f;    // 3.3kΩ
    static constexpr float DIVIDER_RATIO = R2 / (R1 + R2);  // 0.248
    static constexpr float ADC_VREF = 3.3f;                  // V
    static constexpr int ADC_RESOLUTION = 4095;              // 12-bit ADC

    // Filtering
    static constexpr int SAMPLE_COUNT = 10;

    // Convert ADC reading to voltage at ADC pin
    float adcToVoltage(int adcValue) const;

    // Convert ADC voltage to actual battery voltage
    float adcVoltageToBatteryVoltage(float adcVoltage) const;

public:
    /**
     * @brief Construct VoltageSensor
     * @param pin GPIO pin connected to voltage divider (must be ADC-capable)
     */
    VoltageSensor(gpio_num_t pin);

    ~VoltageSensor();

    /**
     * @brief Initialize the sensor
     * @return true if successful, false otherwise
     */
    bool begin();

    /**
     * @brief Read battery voltage in Volts
     * @param averaged If true, returns averaged reading over multiple samples
     * @return Battery voltage in V
     */
    float readVoltage(bool averaged = true);

    /**
     * @brief Read raw ADC value
     * @return ADC value (0-4095)
     */
    int readRawADC();
};
