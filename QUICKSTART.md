# Quick Start Guide - Regenerative Braking System

## 1. Hardware Setup (30 minutes)

### Required Components Checklist
- [x] ESP32 board
- [x] ACS758LCB-50 current sensor
- [ ] 10kΩ and 3.3kΩ resistors (voltage divider)
- [x] 5V relay module
- [x] 3.3V-to-5V level shifter
- [x] BTS7960 motor driver
- [x] 12V DC motor (~100W)
- [x] 12V lead-acid battery
- [x] CC buck-boost converter
- [x] 12V power supply
- [ ] Jumper wires

### Wiring Steps

1. **Connect ESP32 to Current Sensor**
   ```
   ACS758 VCC → ESP32 3.3V
   ACS758 GND → ESP32 GND
   ACS758 OUT → ESP32 GPIO23
   ```

2. **Build Voltage Divider**
   ```
   Battery(+) → R1(10kΩ) → [Junction] → R2(3.3kΩ) → GND
                              ↓
                          GPIO34
   ```

3. **Connect Relay via Level Shifter**
   ```
   ESP32 GPIO15 → Level Shifter LV Input
   ESP32 3.3V   → Level Shifter LV
   5V Supply    → Level Shifter HV
   GND          → Level Shifter GND
   HV Output    → Relay IN
   ```

4. **Connect BTS7960**
   ```
   ESP32 GPIO19 → RPWM
   ESP32 GPIO21 → LPWM
   5V           → R_EN, L_EN, VCC
   12V Supply   → B+
   GND          → B-
   ```

5. **Wire Motor and Relay**
   ```
   Motor T1     → Relay COM
   Relay NO     → BTS7960 M+
   Relay NC     → Buck-Boost IN+
   Motor T2     → BTS7960 M- AND Buck-Boost IN-
   ```

6. **Connect Battery**
   ```
   Battery(+)   → ACS758 IP+ → Voltage Divider → Buck-Boost OUT+
   Battery(-)   → Common GND → Buck-Boost OUT-
   ```

**⚠️ Double-check all connections before powering on!**

## 2. Software Setup (10 minutes)

### Arduino IDE

1. **Install ESP32 Support**
   - Open Arduino IDE
   - File → Preferences
   - Add URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager
   - Install "esp32" by Espressif

2. **Install Libraries**
   - Sketch → Include Library → Manage Libraries
   - Install: **WebSockets** by Markus Sattler

3. **Upload Code**
   - Open `regen_braking.ino`
   - Tools → Board → ESP32 Dev Module
   - Tools → Port → (select your port)
   - Click Upload

### PlatformIO (Alternative)

```bash
cd regen_braking
pio run --target upload
pio device monitor
```

## 3. First Power-On (5 minutes)

1. **Connect ESP32 to USB**
2. **Open Serial Monitor** (115200 baud)
3. **Watch for startup messages**:
   ```
   ========================================
     REGENERATIVE BRAKING SYSTEM
   ========================================

   Current sensor initialized (GPIO 23)
   Voltage sensor initialized (GPIO 34)
   Motor Control initialized

   WiFi Access Point Started
     SSID: regen_braking
     Password: morris123
     IP: 192.168.4.1

   SYSTEM READY
   Web Interface: http://192.168.4.1/
   ========================================
   ```

4. **Connect to WiFi**
   - On phone/laptop: Connect to `regen_braking`
   - Password: `morris123`

5. **Open Web Interface**
   - Browser: `http://192.168.4.1`

## 4. Testing (10 minutes)

### Test 1: Sensor Readings

1. Open web interface
2. Check **Current** reading (~0 mA when motor off)
3. Check **Voltage** reading (should match battery voltage)
4. Verify readings update every 100ms

**Expected**: Current ≈ 0-100 mA (idle), Voltage ≈ 12.0-12.6V

### Test 2: Motor Start

1. Click **START MOTOR** button
2. **Expected**:
   - Relay clicks (audible)
   - Motor spins
   - Current increases (depends on motor)
   - Timer counts down from 30s
   - Button turns green

3. **Monitor Serial**:
   ```
   === Starting Motor ===
   Relay: ON (Motor mode)
   PWM: RPWM=255, LPWM=0
   Motor started at full speed
   Auto-stop in 30 seconds
   ```

### Test 3: Charging Mode

1. Click **CHARGE** button (or wait 30s)
2. **Expected**:
   - Motor stops
   - Relay clicks
   - State shows "CHARGING"
   - Current may go negative (generating)
   - Button turns red

3. **Monitor Serial**:
   ```
   === Starting Charging ===
   PWM: RPWM=0, LPWM=0
   Relay: OFF (Charging mode)
   Charging mode activated
   ```

### Test 4: Auto-Stop Timer

1. Click START MOTOR
2. Wait for 30-second countdown
3. **Expected**: Automatically switches to CHARGING at 0s

### Test 5: Overcurrent Protection

⚠️ **Optional - use caution**

1. Stall motor (hold shaft while running)
2. **Expected**: Immediately stops if current > 40A
3. **Serial shows**: `!!! OVERCURRENT DETECTED !!!`

## 5. Normal Operation

### To Run Motor:
1. Open `http://192.168.4.1`
2. Click **START MOTOR**
3. Motor runs for up to 30 seconds
4. Monitor current and voltage in real-time

### To Charge Battery:
1. Click **CHARGE** button
2. Motor generates electricity
3. Buck-boost charges battery
4. Monitor charging current

### Safety Notes:
- ✅ Motor auto-stops after 30 seconds
- ✅ Overcurrent protection at 40A
- ✅ Real-time monitoring prevents damage
- ✅ Manual stop available anytime

## 6. Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi not appearing | Check Serial Monitor, power cycle ESP32 |
| Current reads 0 | Verify ACS758 powered by 3.3V, check GPIO23 |
| Voltage reads wrong | Check resistor values, verify GPIO34 |
| Motor doesn't start | Check relay wiring, BTS7960 enable pins |
| WebSocket disconnects | Move closer to ESP32, check power stability |
| Overcurrent triggers instantly | Check motor for stall, verify sensor wiring |

### Quick Diagnostics

**Serial Monitor Commands** (check output):
- Look for "ERROR:" messages
- Verify all "initialized" messages appear
- Check IP address matches 192.168.4.1

**Hardware Checks**:
1. Measure ACS758 output: Should be ~1.65V at zero current
2. Measure voltage divider: Should be ~3.0V at 12V battery
3. Test relay: GPIO15 HIGH should activate relay
4. Check motor: Should spin freely by hand

## 7. Advanced Configuration

### Change WiFi Credentials
Edit `regen_braking.ino`:
```cpp
const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASSWORD = "your_password";
```

### Adjust Auto-Stop Timer
Edit `MotorControl.h`:
```cpp
static constexpr unsigned long MOTOR_TIMEOUT_MS = 30000;  // milliseconds
```

### Adjust Overcurrent Threshold
Edit `MotorControl.h`:
```cpp
static constexpr float OVERCURRENT_THRESHOLD_MA = 40000.0f;  // mA
```

### Change Update Rate
Edit `WebServerControl.h`:
```cpp
static constexpr unsigned long WEBSOCKET_UPDATE_INTERVAL = 100;  // ms
```

## 8. Next Steps

- [x] Basic setup complete
- [ ] Fine-tune overcurrent threshold for your motor
- [ ] Calibrate sensors if needed (see README)
- [ ] Add data logging (future enhancement)
- [ ] Optimize charging parameters

## Support

- Check `README.md` for detailed documentation
- Review `WIRING.txt` for connection reference
- Monitor Serial output for diagnostics
- Verify all components are rated for your application

---

**Ready to go!** The system is now operational. Connect to the WiFi AP and enjoy your regenerative braking demo!
