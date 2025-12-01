/**
 * @file regen_braking.ino
 * @brief Regenerative Braking System for ESP32
 *
 * Simulates regenerative braking on a small scale using:
 * - ESP32 microcontroller
 * - ACS758LCB-50 current sensor (GPIO23)
 * - Voltage divider for battery voltage (GPIO34)
 * - 5V relay via level shifter (GPIO15)
 * - BTS7960 motor driver (RPWM=GPIO19, LPWM=GPIO21)
 * - 12V lead-acid battery
 * - CC buck-boost converter for charging
 *
 * Features:
 * - Real-time current (mA) and voltage (V) monitoring
 * - Web interface with 100ms WebSocket updates
 * - Motor control: START and CHARGE buttons
 * - Auto-stop after 30 seconds
 * - Overcurrent protection at 40A
 * - WiFi Access Point: SSID="regen_braking", Password="morris123"
 *
 * @author Your Name
 * @date 2025
 */

#include "CurrentSensor.h"
#include "VoltageSensor.h"
#include "MotorControl.h"
#include "WebServerControl.h"

// ============================================================================
// GPIO Pin Definitions
// ============================================================================

// Sensors
#define PIN_CURRENT_SENSOR  GPIO_NUM_23  // ACS758LCB-50 current sensor
#define PIN_VOLTAGE_SENSOR  GPIO_NUM_34  // Voltage divider (input-only ADC)

// Motor Control
#define PIN_RELAY           GPIO_NUM_15  // Relay control (via level shifter)
#define PIN_RPWM            GPIO_NUM_19  // BTS7960 forward PWM
#define PIN_LPWM            GPIO_NUM_21  // BTS7960 reverse PWM (always LOW)

// ============================================================================
// WiFi Configuration
// ============================================================================

const char* WIFI_SSID = "regen_braking";
const char* WIFI_PASSWORD = "morris123";

// ============================================================================
// Global Objects
// ============================================================================

CurrentSensor currentSensor(PIN_CURRENT_SENSOR);
VoltageSensor voltageSensor(PIN_VOLTAGE_SENSOR);
MotorControl motorControl(PIN_RELAY, PIN_RPWM, PIN_LPWM, &currentSensor);
WebServerControl webServer(
    WIFI_SSID,
    WIFI_PASSWORD,
    &currentSensor,
    &voltageSensor,
    &motorControl
);

// ============================================================================
// Setup Function
// ============================================================================

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  REGENERATIVE BRAKING SYSTEM");
    Serial.println("========================================");
    Serial.println();

    // Initialize current sensor
    Serial.println("--- Initializing Current Sensor ---");
    if (!currentSensor.begin()) {
        Serial.println("ERROR: Failed to initialize current sensor");
        Serial.println("System halted");
        while (1) { delay(1000); }
    }

    // Initialize voltage sensor
    Serial.println("\n--- Initializing Voltage Sensor ---");
    if (!voltageSensor.begin()) {
        Serial.println("ERROR: Failed to initialize voltage sensor");
        Serial.println("System halted");
        while (1) { delay(1000); }
    }

    // Initialize motor control
    Serial.println("\n--- Initializing Motor Control ---");
    if (!motorControl.begin()) {
        Serial.println("ERROR: Failed to initialize motor control");
        Serial.println("System halted");
        while (1) { delay(1000); }
    }

    // Initialize web server
    Serial.println("\n--- Initializing Web Server ---");
    if (!webServer.begin(80)) {
        Serial.println("ERROR: Failed to initialize web server");
        Serial.println("System halted");
        while (1) { delay(1000); }
    }

    // System ready
    Serial.println("========================================");
    Serial.println("  SYSTEM READY");
    Serial.println("========================================");
    Serial.printf("  Web Interface: http://%s/\n", webServer.getIPAddress().c_str());
    Serial.printf("  SSID: %s\n", WIFI_SSID);
    Serial.printf("  Password: %s\n", WIFI_PASSWORD);
    Serial.println("========================================\n");

    Serial.println("Connect to the WiFi AP and open the web interface.");
    Serial.println("System operational.\n");
}

// ============================================================================
// Loop Function
// ============================================================================

void loop() {
    // Handle web server requests
    webServer.handleClient();

    // Update motor control (check safety timers and overcurrent)
    motorControl.update();

    // Small delay to prevent watchdog issues
    delay(1);
}
