#include "CurrentSensor.h"

CurrentSensor::CurrentSensor(gpio_num_t pin)
    : m_pin(pin)
    , m_mutex(NULL)
{
}

CurrentSensor::~CurrentSensor() {
    if (m_mutex != NULL) {
        vSemaphoreDelete(m_mutex);
        m_mutex = NULL;
    }
}

bool CurrentSensor::begin() {
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == NULL) {
        Serial.println("ERROR: Failed to create CurrentSensor mutex");
        return false;
    }

    // Configure ADC pin as input
    pinMode(m_pin, INPUT);

    // Configure ADC attenuation for 0-3.3V range
    analogSetAttenuation(ADC_11db);  // 0-3.3V range

    Serial.printf("Current sensor initialized (GPIO %d)\n", m_pin);
    Serial.printf("  Zero current: %.2fV\n", ZERO_CURRENT_VOLTAGE);
    Serial.printf("  Sensitivity: %.1fmV/A\n", SENSITIVITY * 1000.0f);
    Serial.printf("  Range: ±50A\n");

    return true;
}

float CurrentSensor::adcToVoltage(int adcValue) const {
    return (adcValue / static_cast<float>(ADC_RESOLUTION)) * ADC_VREF;
}

float CurrentSensor::voltageToCurrent(float voltage) const {
    // Current (A) = (Vout - Vzero) / Sensitivity
    // Current (mA) = Current (A) * 1000
    float currentAmps = (voltage - ZERO_CURRENT_VOLTAGE) / SENSITIVITY;
    return currentAmps * 1000.0f;  // Convert to mA
}

int CurrentSensor::readRawADC() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        int adcValue = analogRead(m_pin);
        xSemaphoreGive(m_mutex);
        return adcValue;
    }
    return 0;
}

float CurrentSensor::readCurrentMilliAmps(bool averaged) {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        float totalCurrent = 0.0f;
        int samples = averaged ? SAMPLE_COUNT : 1;

        for (int i = 0; i < samples; i++) {
            int adcValue = analogRead(m_pin);
            float voltage = adcToVoltage(adcValue);
            float current = voltageToCurrent(voltage);
            totalCurrent += current;

            if (averaged && i < samples - 1) {
                delay(1);  // Small delay between samples
            }
        }

        xSemaphoreGive(m_mutex);
        return totalCurrent / samples;
    }

    return 0.0f;
}

bool CurrentSensor::isOverCurrent(float thresholdMilliAmps) {
    float current = readCurrentMilliAmps(false);  // Fast, single reading
    return (abs(current) > thresholdMilliAmps);
}
