# Regenerative Braking System

A small-scale regenerative braking simulation system using ESP32, designed to demonstrate energy recovery from a DC motor into a lead-acid battery.

## Features

- **Real-time Monitoring**: Current (mA) and voltage (V) displayed with 100ms WebSocket updates
- **Web Interface**: Clean black-themed control panel accessible via WiFi
- **Motor Control**: Start motor and charge battery with simple buttons
- **Safety Features**:
  - 30-second auto-stop timer
  - 40A overcurrent protection
  - Automatic relay switching
- **WiFi Access Point**: Connect directly to the ESP32 (no router needed)

## Hardware Components

### Required Components

1. **ESP32 Development Board**
2. **ACS758LCB-50 Current Sensor** (±50A, bidirectional)
3. **Voltage Divider Resistors**:
   - R1: 10kΩ
   - R2: 3.3kΩ
4. **5V Relay Module** (controlled via 3.3V-to-5V level shifter)
5. **BTS7960 Motor Driver** (43A H-bridge)
6. **3.3V to 5V Level Shifter** (for relay control)
7. **12V Lead-Acid Battery**
8. **CC Buck-Boost Converter** (for charging)
9. **DC Motor** (12V, ~100W recommended)
10. **Power Supply** (12V for motor driver)

### Pin Configuration

| Component          | ESP32 GPIO | Notes                                    |
|--------------------|------------|------------------------------------------|
| Current Sensor     | GPIO 23    | ACS758LCB-50 output (analog)             |
| Voltage Divider    | GPIO 34    | Battery voltage sensing (input-only ADC) |
| Relay Control      | GPIO 15    | Via 3.3V-to-5V level shifter             |
| BTS7960 RPWM       | GPIO 19    | Forward PWM (0-255)                      |
| BTS7960 LPWM       | GPIO 18    | Reverse PWM (always 0)                   |

## Wiring Diagram

### Current Sensor (ACS758LCB-50)

```
Battery(+) ──────┬──────> Motor/Buck-Boost
                 │
              [ACS758]
                 │
                VCC ──> ESP32 3.3V
                GND ──> ESP32 GND
                OUT ──> ESP32 GPIO23
```

**Important**: The ACS758 must be powered by 3.3V (not 5V) for this configuration.

### Voltage Divider

```
Battery(+) ──┬── R1 (10kΩ) ──┬── R2 (3.3kΩ) ── GND
             │                │
             │                └──> ESP32 GPIO34
             └──> Battery(-)
```

**Voltage Range**: 0-14.4V input → 0-3.3V at GPIO34

### Relay and Motor Driver

```
ESP32 GPIO15 ──> [Level Shifter] ──> Relay Control (5V)

Relay Configuration:
├─ COM: Motor terminal 1
├─ NO:  BTS7960 output (M+)
└─ NC:  Buck-Boost input (+)

Motor terminal 2 connected to:
├─ BTS7960 output (M-) when relay ON
└─ Buck-Boost input (-) when relay OFF
```

### BTS7960 Motor Driver

```
Power:
├─ B+  ──> 12V Power Supply (+)
├─ B-  ──> 12V Power Supply (-)
└─ VCC ──> ESP32 5V (or separate 5V supply)

Control:
├─ RPWM ──> ESP32 GPIO19
├─ LPWM ──> ESP32 GPIO18
├─ R_EN ──> 5V (hardware tied HIGH)
└─ L_EN ──> 5V (hardware tied HIGH)

Output:
├─ M+ ──> Relay NO (normally open)
└─ M- ──> Motor terminal 2
```

## Software Setup

### Arduino IDE Configuration

1. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to: File → Preferences
   - Add to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to: Tools → Board → Boards Manager
   - Search "ESP32" and install

2. **Install Required Libraries**
   - Open: Sketch → Include Library → Manage Libraries
   - Install the following:
     - **WebSockets** by Markus Sattler (for WebSocket support)
     - **WebServer** (built-in with ESP32 core)

3. **Select Board**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module

### Upload Code

1. Open `regen_braking.ino` in Arduino IDE
2. Connect ESP32 via USB
3. Select correct COM port: Tools → Port
4. Click Upload button
5. Open Serial Monitor (115200 baud) to see startup messages

## WiFi Configuration

The system creates a WiFi Access Point:

- **SSID**: `regen_braking`
- **Password**: `morris123`
- **IP Address**: `192.168.4.1` (default ESP32 AP address)

### Connecting to the System

1. Power on the ESP32
2. On your phone/laptop, connect to WiFi network `regen_braking`
3. Enter password: `morris123`
4. Open browser and navigate to: `http://192.168.4.1`

## Web Interface

### Display Elements

- **Current**: Real-time motor current in milliamps (mA)
- **Voltage**: Battery voltage in volts (V)
- **State**: Current system state (RUNNING, CHARGING, STOPPED)
- **Time Remaining**: Countdown timer when motor is running (30s max)

### Control Buttons

- **START MOTOR**:
  - Activates relay (motor connects to BTS7960)
  - Starts motor at full speed
  - Begins 30-second countdown timer
  - Button turns green when active

- **CHARGE**:
  - Stops motor PWM
  - Deactivates relay (motor connects to buck-boost)
  - Motor acts as generator, charging battery
  - Button turns red when active

### Connection Status

- **Green dot** (top-right): WebSocket connected
- **Red dot** (top-right): WebSocket disconnected

## System Operation

### Motor Mode (RUNNING)

1. Click "START MOTOR" button
2. Relay activates (GPIO15 = HIGH)
3. Motor receives full power from BTS7960 (PWM = 255)
4. Current and voltage update in real-time
5. Timer counts down from 30 seconds
6. Auto-stops when:
   - Timer reaches 0, OR
   - Current exceeds 40A (overcurrent protection)

### Charging Mode (CHARGING)

1. Click "CHARGE" button (or wait for auto-stop)
2. Motor PWM stops (GPIO19 = 0)
3. Relay deactivates (GPIO15 = LOW)
4. Motor acts as generator
5. Generated power flows to battery via buck-boost converter
6. Current sensor shows charging current (may be negative)

### Safety Features

#### Auto-Stop Timer (30 seconds)
- Prevents excessive motor runtime
- Automatically switches to CHARGING mode
- Countdown visible on web interface

#### Overcurrent Protection (40A)
- Continuously monitors motor current
- Triggers emergency stop if current > 40A
- Protects motor, driver, and battery

## Serial Monitor Output

Example startup sequence:

```
========================================
  REGENERATIVE BRAKING SYSTEM
========================================

--- Initializing Current Sensor ---
Current sensor initialized (GPIO 23)
  Zero current: 1.65V
  Sensitivity: 40.0mV/A
  Range: ±50A

--- Initializing Voltage Sensor ---
Voltage sensor initialized (GPIO 34)
  Voltage divider: R1=10.0kΩ, R2=3.3kΩ
  Divider ratio: 0.248
  Max input voltage: 13.3V

--- Initializing Motor Control ---

--- Motor Control Initialized ---
  Relay: GPIO 15 (LOW=charging, HIGH=motor)
  RPWM: GPIO 19 (forward PWM)
  LPWM: GPIO 18 (always LOW)
  Auto-stop timeout: 30 seconds
  Overcurrent threshold: 40000 mA (40.0 A)
  Initial state: CHARGING

========================================
  WiFi Access Point Started
========================================
  SSID: regen_braking
  Password: morris123
  IP: 192.168.4.1
========================================

--- Web Server Initialized ---
  HTTP Port: 80
  WebSocket Port: 81
  Web Interface: http://192.168.4.1/
-------------------------------

========================================
  SYSTEM READY
========================================
  Web Interface: http://192.168.4.1/
  SSID: regen_braking
  Password: morris123
========================================

Connect to the WiFi AP and open the web interface.
System operational.
```

## Calibration

### Current Sensor Calibration

The ACS758LCB-50 outputs:
- **Zero current**: 1.65V (Vcc/2 at 3.3V supply)
- **Sensitivity**: 40mV/A

If readings are inaccurate:

1. Measure actual zero-current voltage with multimeter
2. Update `ZERO_CURRENT_VOLTAGE` in `CurrentSensor.h`:
   ```cpp
   static constexpr float ZERO_CURRENT_VOLTAGE = 1.65f;  // Adjust this
   ```

### Voltage Sensor Calibration

With R1=10kΩ and R2=3.3kΩ:
- **Divider ratio**: 0.248
- **Max voltage**: ~13.3V

If readings are inaccurate:

1. Measure actual resistor values with multimeter
2. Update `R1` and `R2` in `VoltageSensor.h`:
   ```cpp
   static constexpr float R1 = 10000.0f;  // Measured value
   static constexpr float R2 = 3300.0f;   // Measured value
   ```

## Troubleshooting

### WiFi AP Not Appearing

- Check Serial Monitor for errors
- Ensure ESP32 has adequate power (USB may be insufficient during motor operation)
- Try power cycling the ESP32

### Current Sensor Reads 0

- Verify ACS758 is powered by 3.3V (not 5V)
- Check GPIO23 connection
- Measure ACS758 output with multimeter (should be ~1.65V at zero current)

### Voltage Reads Incorrectly

- Verify resistor values (10kΩ and 3.3kΩ)
- Check GPIO34 connection
- Ensure battery voltage < 13.3V

### Motor Doesn't Start

- Check BTS7960 enable pins are HIGH
- Verify 12V power supply to BTS7960
- Check relay operation (should hear click)
- Monitor Serial output for errors

### WebSocket Disconnects Frequently

- Move closer to ESP32
- Reduce WiFi interference
- Check ESP32 power supply stability

### Overcurrent Triggers Immediately

- Check motor stall condition
- Verify current sensor wiring
- Adjust threshold in `MotorControl.h` if needed:
  ```cpp
  static constexpr float OVERCURRENT_THRESHOLD_MA = 40000.0f;  // Adjust
  ```

## Code Structure

```
regen_braking/
├── regen_braking.ino       # Main Arduino sketch
├── CurrentSensor.h         # ACS758 current sensor class
├── CurrentSensor.cpp
├── VoltageSensor.h         # Voltage divider sensor class
├── VoltageSensor.cpp
├── MotorControl.h          # BTS7960 + relay control class
├── MotorControl.cpp
├── WebServerControl.h      # Web server + WebSocket class
├── WebServerControl.cpp
└── README.md               # This file
```

## Safety Warnings

⚠️ **IMPORTANT SAFETY INFORMATION**

1. **Electrical Safety**
   - Work with 12V DC systems only
   - Disconnect battery when wiring
   - Use appropriate wire gauges for high current (>10A)
   - Ensure all connections are secure

2. **Motor Safety**
   - Motor can generate high current when used as generator
   - Ensure adequate heat dissipation for BTS7960
   - Don't exceed motor's rated voltage/current

3. **Battery Safety**
   - Lead-acid batteries can produce explosive hydrogen gas
   - Charge in well-ventilated area
   - Don't short-circuit battery terminals
   - Monitor charging voltage (don't exceed 14.4V for 12V battery)

4. **Overcurrent Protection**
   - 40A threshold is for this specific setup
   - Adjust based on your motor specifications
   - Use appropriate fuses in high-current paths

## Future Improvements

- [ ] Add adjustable motor speed control
- [ ] Display charging current separately
- [ ] Log data to SD card or cloud
- [ ] Add battery state-of-charge estimation
- [ ] Implement PID control for charging current
- [ ] Add reverse motor operation (regenerative braking while reversing)
- [ ] Mobile app interface

## License

This project is open-source for educational purposes.

## Author

Created for regenerative braking demonstration and learning.

## Support

For issues or questions, check:
1. Serial Monitor output for error messages
2. Wiring connections
3. Component specifications

---

**Version**: 1.0
**Last Updated**: 2025-01-29
