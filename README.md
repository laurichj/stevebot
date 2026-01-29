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
- **Safety Features**:
  - **Watchdog Timer**: Automatic recovery from system hangs (10-second timeout)
  - **Non-Volatile Storage**: State persistence across power cycles
  - **Manual Override**: Serial command interface for emergency control
  - **Failsafe Relay State**: Relay defaults to OFF on startup/reset

## Installation

1. Install PlatformIO:
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip3 install platformio
   ```

2. Configure your WiFi credentials:
   - Copy `src/secrets.h.template` to `src/secrets.h`
   - Edit `src/secrets.h` and add your WiFi SSID and password
   - Set your timezone offsets (GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC)
   - Note: `secrets.h` is gitignored and will not be committed

3. Build and upload to your ESP32:
   ```bash
   pio run -e esp32 --target upload
   ```

4. Hardware setup:
   - Connect the relay module to GPIO pin 13
   - Connect your misting system to the relay's normally open (NO) terminals
   - Ensure proper power supply for both ESP32 and misting system

## Wiring

- **Relay Control**: GPIO Pin 13
- **Relay to Mister**: Connect misting system to relay's normally open (NO) terminals

## Usage

### Automatic Operation

Once configured and powered on, Stevebot will:
1. Connect to WiFi and synchronize time via NTP
2. Load saved state from non-volatile storage (if available)
3. Automatically mist for 25 seconds every 2 hours during daylight hours (9am-6pm)
4. Save state after each misting cycle to prevent duplicate misting after power cycles

### Manual Control via Serial Commands

Connect to the serial monitor at 115200 baud and send these commands:

- **`STATUS`** - Display detailed scheduler status
  - Current state (WAITING_SYNC, IDLE, MISTING)
  - Enabled/disabled status
  - Last mist time and next scheduled mist
  - Example output:
    ```
    ===== MISTING SCHEDULER STATUS =====
    State: IDLE
    Scheduler enabled: true
    Last mist time: 2026-01-27 14:30:00
    Next mist estimate: 2026-01-27 16:30:00
    ====================================
    ```

- **`ENABLE`** - Enable automatic misting (saved to non-volatile storage)
  - Allows scheduler to run misting cycles automatically
  - State persists across power cycles

- **`DISABLE`** - Disable automatic misting (saved to non-volatile storage)
  - Prevents future automatic misting cycles
  - Current mist cycle (if active) completes normally
  - State persists across power cycles

- **`FORCE_MIST`** - Immediately trigger a mist cycle
  - Bypasses 2-hour interval check
  - Only works when scheduler is enabled
  - Error if already misting or scheduler disabled

Commands are case-insensitive. Unknown commands return an error message.

### Safety Features

#### Watchdog Timer Protection
- The ESP32 watchdog timer monitors the main loop with a 10-second timeout
- If the system hangs for any reason, the watchdog automatically resets the system
- On restart, the system logs the watchdog reset and resumes normal operation
- Relay defaults to OFF after any reset, preventing stuck-on scenarios

#### State Persistence (Non-Volatile Storage)
- All critical state is saved to ESP32 NVS (Non-Volatile Storage):
  - Last mist timestamp
  - Scheduler enabled/disabled status
  - Historical misting data
- State survives power cycles, preventing duplicate misting after unexpected restarts
- On startup, saved state is automatically restored

#### Recovery After Power Loss
When power is restored after an outage:
1. System loads state from NVS
2. Checks last mist time to prevent immediate re-misting
3. Resumes normal 2-hour interval schedule
4. Relay remains OFF until next scheduled time (unless manually triggered)

## Running Tests

This project uses PlatformIO with a hybrid testing approach:
- **Native Tests**: Fast unit tests that run on your development machine (no hardware required)
- **Embedded Tests**: Integration tests that run on the ESP32 hardware

### Quick Start

```bash
# Install PlatformIO (one time setup)
python3 -m venv .venv
source .venv/bin/activate
pip3 install platformio

# Run native unit tests (fast, no hardware needed)
pio test -e native

# Build and upload firmware to ESP32
pio run -e esp32 --target upload

# Monitor serial output from ESP32
pio device monitor
```

### Test Architecture

The codebase is organized for testability:
- **Interfaces** (`ITimeProvider`, `IRelayController`): Abstract dependencies for testing
- **Mocks** (`test/native/mocks/`): Test doubles that simulate hardware behavior
- **Unit Tests** (`test/test_*/`): Verify individual components in isolation
- **Integration**: The main application (`src/main.cpp`) wires everything together

See [test/README.md](test/README.md) for comprehensive testing documentation.

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
- [x] Implement scheduling logic (9am-6pm, every 2 hours)
- [x] Implement 25-second mister activation timer
- [x] Remove existing I2C and WiFi scanning diagnostic code

### Safety & Reliability
- [x] Add failsafe relay state on startup (ensure mister is OFF)
- [x] Implement watchdog timer to prevent relay from sticking ON
- [x] Add manual override capability via serial commands
- [x] Store last activation time in non-volatile memory (NVS)

### Testing
- [x] Test WiFi connection and reconnection logic
- [x] Test NTP time synchronization accuracy
- [x] Test relay activation timing (verify 25-second duration)
- [x] Test scheduling intervals (verify 2-hour spacing)
- [x] Test time window enforcement (9am-6pm only)
- [x] Test state machine transitions (WAITING_SYNC → IDLE → MISTING)
- [x] Create comprehensive unit test suite with mocks
- [x] Test state persistence and recovery (NVS)
- [x] Test enable/disable functionality
- [x] Test force mist command
- [x] Test state recovery after simulated power loss
- [ ] Test behavior across midnight boundary
- [ ] Test power loss recovery on hardware
- [ ] Test watchdog timer reset recovery
- [ ] Perform 24-hour continuous operation test

For hardware testing procedures, see [docs/hardware-testing-guide.md](docs/hardware-testing-guide.md).

### Monitoring & Debugging
- [x] Add serial output for misting events (timestamps)
- [x] Add serial output for next scheduled misting time
- [x] Add WiFi connection status indicators
- [x] Add time synchronization status output

## License

MIT License (SPDX: MIT)
Copyright 2022 Limor Fried for Adafruit Industries
