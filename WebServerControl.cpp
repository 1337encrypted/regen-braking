#include "WebServerControl.h"

// Built-in HTML interface with polling (no external files needed)
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Regenerative Braking Control</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #000;
            color: #fff;
            padding: 20px;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 28px;
            border-bottom: 2px solid #fff;
            padding-bottom: 15px;
        }
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 30px;
        }
        .status-card {
            background-color: #1a1a1a;
            border: 1px solid #fff;
            padding: 20px;
            text-align: center;
        }
        .status-label {
            font-size: 14px;
            color: #aaa;
            margin-bottom: 10px;
        }
        .status-value {
            font-size: 32px;
            font-weight: bold;
            color: #fff;
        }
        .status-unit {
            font-size: 18px;
            color: #aaa;
            margin-left: 5px;
        }
        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-bottom: 20px;
        }
        .speed-control {
            background-color: #1a1a1a;
            border: 1px solid #fff;
            padding: 20px;
            margin-bottom: 20px;
        }
        .speed-label {
            font-size: 14px;
            color: #aaa;
            margin-bottom: 10px;
            display: flex;
            justify-content: space-between;
        }
        .speed-value {
            color: #fff;
            font-weight: bold;
        }
        .slider {
            width: 100%;
            height: 8px;
            background: #333;
            outline: none;
            border: 1px solid #fff;
            -webkit-appearance: none;
        }
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            background: #fff;
            cursor: pointer;
            border: 2px solid #000;
        }
        .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            background: #fff;
            cursor: pointer;
            border: 2px solid #000;
        }
        button {
            padding: 20px;
            font-size: 18px;
            font-weight: bold;
            border: 2px solid #fff;
            cursor: pointer;
            transition: none;
        }
        #btnStart {
            background-color: #1a1a1a;
            color: #fff;
        }
        #btnStart.active {
            background-color: #000;
            color: #0f0;
            border-color: #0f0;
        }
        #btnStop {
            background-color: #1a1a1a;
            color: #fff;
        }
        #btnStop.active {
            background-color: #000;
            color: #f00;
            border-color: #f00;
        }
        button:active {
            transform: scale(0.98);
        }
        .info {
            background-color: #1a1a1a;
            border: 1px solid #fff;
            padding: 15px;
            margin-bottom: 20px;
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
            font-size: 14px;
        }
        .info-label {
            color: #aaa;
        }
        .info-value {
            color: #fff;
            font-weight: bold;
        }
        .state-running {
            color: #0f0 !important;
        }
        .state-charging {
            color: #ff0 !important;
        }
        .state-stopped {
            color: #f00 !important;
        }
        .footer {
            text-align: center;
            margin-top: 30px;
            font-size: 12px;
            color: #666;
        }
        .connection-status {
            position: fixed;
            top: 10px;
            right: 10px;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            background-color: #0f0;
        }
        .connection-status.error {
            background-color: #f00;
        }
    </style>
</head>
<body>
    <div class="connection-status" id="connStatus"></div>
    <div class="container">
        <h1>REGENERATIVE BRAKING</h1>

        <div class="status-grid">
            <div class="status-card">
                <div class="status-label">CURRENT</div>
                <div>
                    <span class="status-value" id="current">0</span>
                    <span class="status-unit">mA</span>
                </div>
            </div>
            <div class="status-card">
                <div class="status-label">VOLTAGE</div>
                <div>
                    <span class="status-value" id="voltage">0.00</span>
                    <span class="status-unit">V</span>
                </div>
            </div>
        </div>

        <div class="speed-control">
            <div class="speed-label">
                <span>MOTOR SPEED</span>
                <span class="speed-value"><span id="speedValue">128</span> (<span id="speedPercent">50</span>%)</span>
            </div>
            <input type="range" min="0" max="255" value="128" class="slider" id="speedSlider" oninput="updateSpeedDisplay(this.value)" onchange="setSpeed(this.value)">
        </div>

        <div class="controls">
            <button id="btnStart" onclick="startMotor()">START MOTOR</button>
            <button id="btnStop" onclick="stopMotor()">CHARGE</button>
        </div>

        <div class="info">
            <div class="info-row">
                <span class="info-label">State:</span>
                <span class="info-value" id="state">-</span>
            </div>
            <div class="info-row">
                <span class="info-label">Time Remaining:</span>
                <span class="info-value" id="timeLeft">-</span>
            </div>
        </div>

        <div class="footer">
            <p>&copy; 2025 Regenerative Braking System</p>
        </div>
    </div>

    <script>
        // Poll status every 100ms
        let pollInterval = null;

        function startPolling() {
            if (pollInterval) return;
            pollInterval = setInterval(updateStatus, 100);
            updateStatus(); // Immediate first update
        }

        function updateStatus() {
            fetch('/api/status')
                .then(response => {
                    if (!response.ok) throw new Error('Network error');
                    return response.json();
                })
                .then(data => {
                    // Update displays
                    document.getElementById('current').textContent = Math.round(data.current);
                    document.getElementById('voltage').textContent = data.voltage.toFixed(2);

                    // Update speed slider (only if not currently being dragged)
                    if (data.speed !== undefined && !document.getElementById('speedSlider').matches(':active')) {
                        document.getElementById('speedSlider').value = data.speed;
                        document.getElementById('speedValue').textContent = data.speed;
                        document.getElementById('speedPercent').textContent = Math.round((data.speed / 255) * 100);
                    }

                    // Update state
                    const stateEl = document.getElementById('state');
                    stateEl.textContent = data.state.toUpperCase();
                    stateEl.className = 'info-value state-' + data.state;

                    // Update time remaining
                    const timeLeftEl = document.getElementById('timeLeft');
                    if (data.timeLeft > 0) {
                        timeLeftEl.textContent = data.timeLeft + 's';
                    } else {
                        timeLeftEl.textContent = '-';
                    }

                    // Update button states
                    const btnStart = document.getElementById('btnStart');
                    const btnStop = document.getElementById('btnStop');

                    if (data.state === 'running') {
                        btnStart.classList.add('active');
                        btnStop.classList.remove('active');
                    } else if (data.state === 'charging') {
                        btnStart.classList.remove('active');
                        btnStop.classList.add('active');
                    } else {
                        btnStart.classList.remove('active');
                        btnStop.classList.remove('active');
                    }

                    // Connection OK
                    document.getElementById('connStatus').classList.remove('error');
                })
                .catch(error => {
                    console.error('Status update error:', error);
                    document.getElementById('connStatus').classList.add('error');
                });
        }

        function startMotor() {
            fetch('/api/start')
                .then(response => response.text())
                .then(data => console.log('Start:', data))
                .catch(error => console.error('Error:', error));
        }

        function stopMotor() {
            fetch('/api/stop')
                .then(response => response.text())
                .then(data => console.log('Stop:', data))
                .catch(error => console.error('Error:', error));
        }

        function updateSpeedDisplay(value) {
            document.getElementById('speedValue').textContent = value;
            document.getElementById('speedPercent').textContent = Math.round((value / 255) * 100);
        }

        function setSpeed(value) {
            fetch('/api/speed?value=' + value)
                .then(response => response.text())
                .then(data => console.log('Speed set:', data))
                .catch(error => console.error('Error:', error));
        }

        // Start polling when page loads
        startPolling();
    </script>
</body>
</html>
)rawliteral";

// Constructor
WebServerControl::WebServerControl(
    const char* ssid,
    const char* password,
    CurrentSensor* currentSensor,
    VoltageSensor* voltageSensor,
    MotorControl* motorControl
)
    : m_currentSensor(currentSensor)
    , m_voltageSensor(voltageSensor)
    , m_motorControl(motorControl)
    , m_server(nullptr)
    , m_ssid(ssid)
    , m_password(password)
    , m_initialized(false)
    , m_wifiConnected(false)
    , m_mutex(NULL)
{
}

// Destructor
WebServerControl::~WebServerControl() {
    if (m_server != nullptr) {
        delete m_server;
        m_server = nullptr;
    }
    if (m_mutex != NULL) {
        vSemaphoreDelete(m_mutex);
        m_mutex = NULL;
    }
}

bool WebServerControl::setupAccessPoint() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);

    WiFi.mode(WIFI_AP);
    delay(500);

    bool apStarted = WiFi.softAP(m_ssid, m_password);

    if (apStarted) {
        delay(1000);
        m_wifiConnected = true;
        return true;
    }

    return false;
}

bool WebServerControl::begin(uint16_t port) {
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (m_mutex == NULL) {
        Serial.println("ERROR: Failed to create WebServerControl mutex");
        return false;
    }

    // Setup WiFi AP
    if (!setupAccessPoint()) {
        Serial.println("ERROR: WiFi AP setup failed");
        return false;
    }

    Serial.println("\n========================================");
    Serial.println("  WiFi Access Point Started");
    Serial.println("========================================");
    Serial.printf("  SSID: %s\n", m_ssid);
    Serial.printf("  Password: %s\n", m_password);
    Serial.printf("  IP: %s\n", getIPAddress().c_str());
    Serial.println("========================================\n");

    // Create HTTP server
    m_server = new WebServer(port);
    if (m_server == nullptr) {
        Serial.println("ERROR: Failed to allocate WebServer");
        return false;
    }

    // Setup routes and start server
    setupRoutes();
    m_server->begin();

    Serial.println("--- Web Server Initialized ---");
    Serial.printf("  HTTP Port: %d\n", port);
    Serial.printf("  Web Interface: http://%s/\n", getIPAddress().c_str());
    Serial.println("-------------------------------\n");

    m_initialized = true;
    return true;
}

void WebServerControl::setupRoutes() {
    // Serve root page
    m_server->on("/", HTTP_GET, [this]() { handleRoot(); });

    // API endpoints
    m_server->on("/api/start", HTTP_GET, [this]() { handleStart(); });
    m_server->on("/api/stop", HTTP_GET, [this]() { handleStop(); });
    m_server->on("/api/speed", HTTP_GET, [this]() { handleSpeed(); });
    m_server->on("/api/status", HTTP_GET, [this]() { handleStatus(); });

    // Not found handler
    m_server->onNotFound([this]() { handleNotFound(); });
}

void WebServerControl::handleRoot() {
    m_server->send_P(200, "text/html", INDEX_HTML);
}

void WebServerControl::handleStart() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool success = m_motorControl->startMotor();
        xSemaphoreGive(m_mutex);

        if (success) {
            m_server->send(200, "text/plain", "Motor started");
        } else {
            m_server->send(500, "text/plain", "Failed to start motor");
        }
    } else {
        m_server->send(500, "text/plain", "Mutex timeout");
    }
}

void WebServerControl::handleStop() {
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool success = m_motorControl->startCharging();
        xSemaphoreGive(m_mutex);

        if (success) {
            m_server->send(200, "text/plain", "Charging started");
        } else {
            m_server->send(500, "text/plain", "Failed to start charging");
        }
    } else {
        m_server->send(500, "text/plain", "Mutex timeout");
    }
}

void WebServerControl::handleSpeed() {
    if (!m_server->hasArg("value")) {
        m_server->send(400, "text/plain", "Missing speed value");
        return;
    }

    int speed = m_server->arg("value").toInt();

    if (speed < 0 || speed > 255) {
        m_server->send(400, "text/plain", "Speed must be 0-255");
        return;
    }

    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool success = m_motorControl->setSpeed((uint8_t)speed);
        xSemaphoreGive(m_mutex);

        if (success) {
            m_server->send(200, "text/plain", "Speed set to " + String(speed));
        } else {
            m_server->send(500, "text/plain", "Failed to set speed");
        }
    } else {
        m_server->send(500, "text/plain", "Mutex timeout");
    }
}

void WebServerControl::handleStatus() {
    String json = "{";

    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        float current = m_currentSensor->readCurrentMilliAmps(false);
        float voltage = m_voltageSensor->readVoltage(false);
        String state = m_motorControl->getStateString();
        unsigned long timeLeft = m_motorControl->getTimeRemaining();
        uint8_t speed = m_motorControl->getSpeed();

        json += "\"current\":" + String(current, 0);
        json += ",\"voltage\":" + String(voltage, 2);
        json += ",\"state\":\"" + state + "\"";
        json += ",\"timeLeft\":" + String(timeLeft);
        json += ",\"speed\":" + String(speed);

        xSemaphoreGive(m_mutex);
    }

    json += "}";
    m_server->sendHeader("Cache-Control", "no-cache");
    m_server->send(200, "application/json", json);
}

void WebServerControl::handleNotFound() {
    m_server->send(404, "text/plain", "Not Found");
}

void WebServerControl::handleClient() {
    if (m_server != nullptr) {
        m_server->handleClient();
    }
}

String WebServerControl::getIPAddress() const {
    if (!m_wifiConnected) {
        return "Not Connected";
    }
    return WiFi.softAPIP().toString();
}
