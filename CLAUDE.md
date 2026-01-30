# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commit Guidelines

**IMPORTANT**: Do NOT add "Co-Authored-By: Claude" or any Claude Code attribution text to commit messages unless explicitly instructed by the user.

## Project Overview

This is a PlatformIO-based ESP32 project for automated iguana habitat environmental control. The project name is "stevebot". It controls a relay-connected misting system to maintain proper humidity levels on a scheduled basis.

## Hardware Requirements

- ESP32 microcontroller (Adafruit Feather ESP32-S2 or compatible)
- Relay module connected to GPIO pin 13
- Misting system (connected via relay)
- WiFi network (for NTP time synchronization)
- Power supply

## Architecture

The project uses a modular, object-oriented architecture with dependency injection for testability:

### Core Components

- **`src/main.cpp`**: Entry point with setup(), loop(), and serial command processing
- **`src/MistingScheduler`**: State machine managing misting cycles (9am-6pm, 2-hour intervals)
- **`src/NTPTimeProvider`**: WiFi and NTP time synchronization implementation
- **`src/GPIORelayController`**: Hardware relay control implementation
- **`src/NVSStateStorage`**: ESP32 non-volatile storage for state persistence

### Interfaces (for testing)

- **`src/ITimeProvider.h`**: Time abstraction (allows mock time in tests)
- **`src/IRelayController.h`**: Relay abstraction (allows mock relay in tests)
- **`src/IStateStorage.h`**: Storage abstraction (allows mock NVS in tests)

### Safety Features

- **Watchdog Timer**: 10-second timeout, automatic system reset on hang
- **State Persistence**: NVS storage survives power cycles, prevents duplicate misting
- **Serial Commands**: Manual override via ENABLE, DISABLE, FORCE_MIST, STATUS
- **Failsafe Relay**: Defaults to OFF on startup/reset

## Dependencies

All dependencies are managed via PlatformIO:

- **WiFi** (built-in ESP32 library): Network connectivity for NTP
- **Preferences** (built-in ESP32 library): Non-volatile storage (NVS)
- **Unity** (testing framework): Native unit tests

No external libraries required.

## Building and Uploading

This project uses **PlatformIO** (not Arduino IDE) with a **Makefile** for common tasks.

### Quick Start

```bash
# One-time setup: Install PlatformIO and configure secrets
make setup

# Run tests (fast, no hardware required)
make test

# Build and upload firmware
make flash   # Builds, uploads, and opens serial monitor

# View all available commands
make help
```

### Manual PlatformIO Commands (Alternative)

If you prefer direct `pio` commands:

```bash
# Build firmware for ESP32
pio run -e esp32

# Upload to connected ESP32
pio run -e esp32 --target upload

# Monitor serial output (115200 baud)
pio device monitor --filter send_on_enter
```

## Running Tests

The project uses a **hybrid testing approach**:

### Native Unit Tests (Recommended for Development)

Fast tests that run on your development machine without hardware:

```bash
# Run all native tests (38 tests, ~5 seconds)
make test

# Run with verbose output
make test-verbose

# Run specific test suite
make test-specific TEST=test_state_machine

# Verify build and tests pass
make verify
```

**Test Coverage**: 38 unit tests covering:
- Time window enforcement (9am-6pm)
- State machine transitions
- 2-hour interval timing
- State persistence and recovery
- Enable/disable functionality
- Force mist command
- Mock infrastructure

### Embedded Tests (Hardware Required)

Integration tests that run on actual ESP32:

```bash
# Requires secrets.h with WiFi credentials
pio test -e embedded --target upload
pio device monitor -b 115200 --filter send_on_enter
```

See `test/README.md` for detailed test documentation.

## Code Architecture Details

### Main Loop (src/main.cpp)

```cpp
void setup() {
    // 1. Initialize watchdog timer
    // 2. Connect to WiFi
    // 3. Synchronize time via NTP
    // 4. Load saved state from NVS
}

void loop() {
    // 1. Feed watchdog (prove system is alive)
    // 2. Process serial commands (non-blocking)
    // 3. Update scheduler state machine
}
```

### Scheduler State Machine

- **WAITING_SYNC**: Waiting for NTP time synchronization
- **IDLE**: Time synced, waiting for next misting window
- **MISTING**: Actively running mister (25 seconds)

### Serial Commands

Connect serial monitor at 115200 baud and send:

- **`STATUS`**: Display scheduler state, last/next mist times
- **`ENABLE`**: Enable automatic misting (persists to NVS)
- **`DISABLE`**: Disable automatic misting (persists to NVS)
- **`FORCE_MIST`**: Manually trigger immediate mist cycle

Commands are case-insensitive.

## Serial Output

The system outputs timestamped logs at 115200 baud:

```
2026-01-28 14:30:00 | MIST START
2026-01-28 14:30:25 | MIST STOP
2026-01-28 14:30:25 | Scheduler ENABLED
```

Connect via serial monitor to view:
- WiFi connection status
- NTP synchronization status
- Misting events with timestamps
- State changes (enable/disable)
- Watchdog reset warnings (if system recovered from hang)
- Serial command responses

## Development Workflow

1. **Make code changes** in `src/` or test files
2. **Run native tests**: `make test` (fast feedback, no hardware)
3. **Build for ESP32**: `make build` (verify compilation)
4. **Upload to hardware**: `make flash` (upload + serial monitor)
5. **Test on device**: Use serial commands (STATUS, ENABLE, etc.)
6. **Commit changes**: Follow git commit guidelines above

### Common Makefile Commands

- `make help` - Show all available commands
- `make test` - Run unit tests
- `make build` - Build firmware
- `make upload` - Upload to ESP32
- `make monitor` - Open serial monitor
- `make flash` - Build, upload, and monitor
- `make clean` - Clean build artifacts
- `make verify` - Run tests and build (CI-friendly)

## Additional Documentation

- **README.md**: User-facing documentation, installation, features
- **test/README.md**: Comprehensive test suite documentation
- **docs/hardware-testing-guide.md**: Hardware testing procedures
- **docs/plans/**: Design documents and implementation plans
