#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "CurrentSensor.h"
#include "VoltageSensor.h"
#include "MotorControl.h"

/**
 * @brief Web Server Control Class for Regenerative Braking System
 *
 * Provides web interface with:
 * - WiFi Access Point mode
 * - REST API for motor control
 * - Polling-based status updates (100ms client-side polling)
 * - Built-in HTML interface (no external files needed)
 */
class WebServerControl {
private:
    // Hardware component references
    CurrentSensor* m_currentSensor;
    VoltageSensor* m_voltageSensor;
    MotorControl* m_motorControl;

    // Web server instance
    WebServer* m_server;

    // WiFi credentials
    const char* m_ssid;
    const char* m_password;

    // Server state
    bool m_initialized;
    bool m_wifiConnected;

    // Mutex for thread-safe access
    SemaphoreHandle_t m_mutex;

    // WiFi setup
    bool setupAccessPoint();

    // Route handlers
    void setupRoutes();
    void handleRoot();
    void handleStart();
    void handleStop();
    void handleStatus();
    void handleNotFound();

public:
    /**
     * @brief Construct WebServerControl
     * @param ssid WiFi AP SSID
     * @param password WiFi AP password
     * @param currentSensor Pointer to current sensor
     * @param voltageSensor Pointer to voltage sensor
     * @param motorControl Pointer to motor control
     */
    WebServerControl(
        const char* ssid,
        const char* password,
        CurrentSensor* currentSensor,
        VoltageSensor* voltageSensor,
        MotorControl* motorControl
    );

    ~WebServerControl();

    /**
     * @brief Initialize web server
     * @param port HTTP server port (default 80)
     * @return true if successful, false otherwise
     */
    bool begin(uint16_t port = 80);

    /**
     * @brief Handle client requests - call in loop()
     */
    void handleClient();

    /**
     * @brief Check if WiFi is connected
     * @return true if connected
     */
    bool isWiFiConnected() const { return m_wifiConnected; }

    /**
     * @brief Get IP address
     * @return IP address string
     */
    String getIPAddress() const;

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }
};
