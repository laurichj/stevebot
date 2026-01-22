# Stevebot Test Suite

This directory contains unit tests for the Stevebot environmental controller.

## Test Files

- `test_main.ino` - Main test runner sketch
- `test_wifi_ntp.cpp` - WiFi and NTP functionality tests

## Running Tests

### Using Arduino IDE

1. **Prepare secrets file**:
   ```bash
   cp ../secrets.h.template ../secrets.h
   # Edit secrets.h with your WiFi credentials and timezone
   ```

2. **Open the test sketch**:
   - Open `test_main.ino` in Arduino IDE
   - The IDE will create a project folder and include all `.cpp` files in the `test/` directory

3. **Upload and run**:
   - Select your ESP32 board and port
   - Upload the sketch
   - Open Serial Monitor at 115200 baud
   - Tests will run automatically on startup

### Using Arduino CLI

```bash
# Compile tests
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 test/

# Upload tests
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:adafruit_feather_esp32s2 test/

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Test Coverage

### WiFi Tests
1. **WiFi mode configuration** - Verifies WiFi is set to station mode
2. **Initial connection** - Tests connection to configured WiFi network
3. **IP address assignment** - Verifies valid IP address obtained
4. **Signal strength** - Checks RSSI is in valid range
5. **Disconnection** - Tests ability to disconnect
6. **Reconnection** - Tests ability to reconnect after disconnection
7. **Timeout handling** - Verifies connection attempts timeout appropriately

### NTP Tests
1. **NTP configuration** - Verifies NTP client can be configured
2. **Time synchronization** - Tests successful sync with NTP server
3. **Time structure validation** - Verifies all time fields are in valid ranges
4. **Time accuracy** - Confirms time progresses correctly
5. **Timezone application** - Verifies timezone offset is applied

## Expected Output

```
========================================
  STEVEBOT WiFi & NTP TEST SUITE
========================================

[TEST] WiFi Connection Test
[PASS] WiFi mode set to STA
[PASS] WiFi begin initiated
[PASS] WiFi connected successfully
[PASS] Valid IP address obtained
   IP Address: 192.168.1.100
[PASS] Signal strength in valid range
   RSSI: -45 dBm

[TEST] WiFi Reconnection Test
[PASS] WiFi disconnected
[PASS] WiFi reconnected successfully
[PASS] Reconnection within timeout period

[TEST] NTP Synchronization Test
[PASS] NTP configuration initiated
[PASS] NTP time synchronized
[PASS] Year is valid (2024 or later)
[PASS] Month is valid (0-11)
[PASS] Day is valid (1-31)
[PASS] Hour is valid (0-23)
[PASS] Minute is valid (0-59)
[PASS] Second is valid (0-59)
   Synchronized time: 2026-1-21 17:30:45

[TEST] NTP Time Accuracy Test
[PASS] Time progresses correctly (2 sec +/- 1)
   Time difference: 2 seconds

[TEST] NTP Timezone Configuration Test
[PASS] Hour is in valid range with timezone applied
   Configured GMT offset: -8 hours
   Configured DST offset: 1 hours

========== TEST SUMMARY ==========
Tests Run: 13
Tests Passed: 13
Tests Failed: 0
==================================

✓ ALL TESTS PASSED!
```

## Troubleshooting

### WiFi Connection Failures
- Verify `secrets.h` has correct SSID and password
- Check that WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure WiFi network is in range

### NTP Synchronization Failures
- Verify WiFi is connected first
- Check that network allows NTP traffic (UDP port 123)
- Try alternative NTP servers (edit `test_wifi_ntp.cpp`)
- Ensure router firewall isn't blocking NTP

### Time Accuracy Issues
- Verify timezone offsets in `secrets.h` are correct
- Check for clock drift over longer periods
- Consider adjusting NTP server if geographically distant
