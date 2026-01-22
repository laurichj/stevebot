# Stevebot

An Arduino-based environmental controller for iguana habitats. Stevebot automatically maintains proper humidity levels by triggering a misting system at scheduled intervals throughout the day.

## Overview

Stevebot uses an ESP32 microcontroller to control a relay-connected misting system, ensuring your iguana's enclosure maintains appropriate humidity levels. The system runs autonomously, activating the mister for 25 seconds every 2 hours during daylight hours (9am to 6pm).

## Hardware Requirements

- ESP32 development board (Adafruit Feather ESP32-S2 or similar)
- Relay module (connected to pin 13)
- Misting system (connected via relay)
- WiFi network (for time synchronization via NTP)
- Power supply

## Features

- **Automated Misting Schedule**: Runs mister for 25 seconds every 2 hours
- **Daylight Hours Operation**: Active only between 9am and 6pm
- **NTP Time Synchronization**: Uses WiFi to maintain accurate time
- **Diagnostic Capabilities**: I2C and WiFi scanning for troubleshooting

## Installation

1. Install the Arduino IDE or Arduino CLI
2. Install required libraries:
   - Adafruit_TestBed
   - WiFi (built-in for ESP32)
3. Configure your WiFi credentials:
   - Copy `secrets.h.template` to `secrets.h`
   - Edit `secrets.h` and add your WiFi SSID and password
   - Note: `secrets.h` is gitignored and will not be committed
4. Upload the sketch to your ESP32 board
5. Connect the relay to pin 13
6. Connect your misting system to the relay

## Wiring

- **Relay Control**: GPIO Pin 13
- **Relay to Mister**: Connect misting system to relay's normally open (NO) terminals

## Running Tests

Comprehensive unit tests for WiFi and NTP functionality are available in the `test/` directory.

See [test/README.md](test/README.md) for detailed instructions on running the test suite.

Quick start:
```bash
# Using Arduino IDE: Open test/test_main.ino and upload
# Using Arduino CLI:
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 test/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:adafruit_feather_esp32s2 test/
```

## Testing NTP Time Synchronization (Manual)

After uploading the main sketch, open the Serial Monitor at 115200 baud. You should see:

1. **WiFi Connection Test**: Verify that the board connects to your WiFi network and receives an IP address
2. **NTP Synchronization Test**: Confirm that time is synchronized with the NTP server
3. **Time Display Test**: Current time is displayed every 10 seconds to verify accuracy

Expected serial output:
```
Connecting to WiFi: YourNetworkName
....
WiFi connected!
IP address: 192.168.1.100
Synchronizing time with NTP server...
....
Time synchronized!
Current time: Tuesday, January 21 2026 17:30:45
...
Current time: 2026-01-21 17:30:55
```

If time synchronization fails, verify:
- WiFi connection is stable
- Your network allows NTP traffic (UDP port 123)
- Timezone offsets in `secrets.h` are correct

## TODO

### Core Functionality
- [x] Add WiFi credentials configuration (SSID and password)
- [x] Implement NTP client for time synchronization
- [x] Add timezone configuration support
- [ ] Implement scheduling logic (9am-6pm, every 2 hours)
- [ ] Implement 25-second mister activation timer
- [ ] Remove existing I2C and WiFi scanning diagnostic code

### Safety & Reliability
- [ ] Add failsafe relay state on startup (ensure mister is OFF)
- [ ] Implement watchdog timer to prevent relay from sticking ON
- [ ] Add manual override capability via serial commands
- [ ] Store last activation time in non-volatile memory (EEPROM/preferences)

### Testing
- [x] Test WiFi connection and reconnection logic
- [x] Test NTP time synchronization accuracy
- [ ] Test relay activation timing (verify 25-second duration)
- [ ] Test scheduling intervals (verify 2-hour spacing)
- [ ] Test time window enforcement (9am-6pm only)
- [ ] Test behavior across midnight boundary
- [ ] Test power loss recovery and time resync
- [ ] Test manual override commands
- [ ] Perform 24-hour continuous operation test

### Monitoring & Debugging
- [ ] Add serial output for misting events (timestamps)
- [ ] Add serial output for next scheduled misting time
- [ ] Add WiFi connection status indicators
- [ ] Add time synchronization status output

## License

MIT License (SPDX: MIT)
Copyright 2022 Limor Fried for Adafruit Industries
