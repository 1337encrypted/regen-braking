#include "VoltageSensor.h"

VoltageSensor::VoltageSensor(gpio_num_t pin)
    : m_pin(pin)
    , m_mutex(NULL)
{
}

VoltageSensor::~VoltageSensor() {
    if (m_mutex != NULL) {
        vSemaphoreDelete(m_mutex);
        m_mutex = NULL;
    }
}

bool VoltageSensor::begin() {
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == NULL) {
        Serial.println("ERROR: Failed to create VoltageSensor mutex");
        return false;
    }

    // Configure ADC pin as input
    pinMode(m_pin, INPUT);

    // Configure ADC attenuation for 0-3.3V range
    analogSetAttenuation(ADC_11db);  // 0-3.3V range

    Serial.printf("Voltage sensor initialized (GPIO %d)\n", m_pin);
    Serial.printf("  Voltage divider: R1=%.1fkΩ, R2=%.1fkΩ\n", R1/1000.0f, R2/1000.0f);
    Serial.printf("  Divider ratio: %.3f\n", DIVIDER_RATIO);
    Serial.printf("  Max input voltage: %.1fV\n", ADC_VREF / DIVIDER_RATIO);

    return true;
}

float VoltageSensor::adcToVoltage(int adcValue) const {
    return (adcValue / static_cast<float>(ADC_RESOLUTION)) * ADC_VREF;
}

float VoltageSensor::adcVoltageToBatteryVoltage(float adcVoltage) const {
    // Battery Voltage = ADC Voltage / Divider Ratio
    return adcVoltage / DIVIDER_RATIO;
}

int VoltageSensor::readRawADC() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        int adcValue = analogRead(m_pin);
        xSemaphoreGive(m_mutex);
        return adcValue;
    }
    return 0;
}

float VoltageSensor::readVoltage(bool averaged) {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        float totalVoltage = 0.0f;
        int samples = averaged ? SAMPLE_COUNT : 1;

        for (int i = 0; i < samples; i++) {
            int adcValue = analogRead(m_pin);
            float adcVoltage = adcToVoltage(adcValue);
            float batteryVoltage = adcVoltageToBatteryVoltage(adcVoltage);
            totalVoltage += batteryVoltage;

            if (averaged && i < samples - 1) {
                delay(1);  // Small delay between samples
            }
        }

        xSemaphoreGive(m_mutex);
        return totalVoltage / samples;
    }

    return 0.0f;
}
