# Test Infrastructure Design

**Date:** 2026-01-22
**Status:** Approved
**Purpose:** Add comprehensive automated testing for misting scheduler with hybrid native/embedded test approach

## Overview

This design adds a robust test infrastructure for the stevebot misting scheduler using a hybrid approach: fast native unit tests for logic verification and on-device integration tests for timing accuracy and hardware validation.

## Requirements

- Test time window enforcement (9am-6pm)
- Test interval timing (2-hour spacing)
- Test state machine transitions
- Test relay activation timing accuracy
- Maintain existing on-device test framework
- Enable fast test-driven development with native tests
- Preserve Arduino workflow compatibility

## Architecture

### Core Components

1. **MistingScheduler class** - Encapsulates all scheduling logic and state machine
2. **ITimeProvider interface** - Abstract time access (real NTP time on device, mockable in tests)
3. **IRelayController interface** - Abstract relay control (real GPIO on device, mockable in tests)
4. **On-device implementations** - `NTPTimeProvider` and `GPIORelayController`
5. **Test mocks** - `MockTimeProvider` and `MockRelayController` for native tests

**Key Design Principle:** The `MistingScheduler` knows nothing about WiFi, NTP, or GPIO. It only knows about abstract interfaces, making it 100% testable in isolation.

### Directory Structure

```
stevebot/
├── platformio.ini          # PlatformIO configuration
├── src/
│   ├── main.cpp            # Main entry (Arduino setup/loop)
│   ├── MistingScheduler.h
│   ├── MistingScheduler.cpp
│   ├── ITimeProvider.h
│   ├── IRelayController.h
│   ├── NTPTimeProvider.h
│   ├── NTPTimeProvider.cpp
│   ├── GPIORelayController.h
│   └── GPIORelayController.cpp
├── test/
│   ├── native/             # Native unit tests (run on computer)
│   │   ├── test_time_window.cpp
│   │   ├── test_state_machine.cpp
│   │   ├── test_interval_timing.cpp
│   │   └── mocks/
│   │       ├── MockTimeProvider.h
│   │       └── MockRelayController.h
│   └── embedded/           # On-device integration tests
│       └── test_timing_accuracy.cpp
└── stevebot.ino           # Legacy file (can be removed after migration)
```

## Interface Definitions

### ITimeProvider Interface

```cpp
// ITimeProvider.h
#ifndef I_TIME_PROVIDER_H
#define I_TIME_PROVIDER_H

#include <time.h>

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;

    // Returns true if time is available and sets timeinfo
    virtual bool getTime(struct tm* timeinfo) = 0;

    // Returns current millis() value for duration tracking
    virtual unsigned long getMillis() = 0;
};

#endif
```

**Design rationale:**
- Abstracts both real-time clock (`getTime`) and duration tracking (`getMillis`)
- No ESP32-specific dependencies
- Easy to mock for testing different time scenarios

### IRelayController Interface

```cpp
// IRelayController.h
#ifndef I_RELAY_CONTROLLER_H
#define I_RELAY_CONTROLLER_H

class IRelayController {
public:
    virtual ~IRelayController() = default;

    // Turn relay on (activate mister)
    virtual void turnOn() = 0;

    // Turn relay off (deactivate mister)
    virtual void turnOff() = 0;
};

#endif
```

**Design rationale:**
- Simple on/off semantics
- Hardware-agnostic
- Easy to track state changes in tests

## MistingScheduler Class

### Header (MistingScheduler.h)

```cpp
#ifndef MISTING_SCHEDULER_H
#define MISTING_SCHEDULER_H

#include "ITimeProvider.h"
#include "IRelayController.h"

enum MisterState {
  WAITING_SYNC,   // Time not yet synchronized
  IDLE,           // Waiting for next misting time
  MISTING         // Actively running the mister
};

class MistingScheduler {
public:
    MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController);

    // Call from main loop
    void update();

    // Query current state
    MisterState getState() const { return currentState; }
    unsigned long getLastMistTime() const { return lastMistTime; }
    unsigned long getMistStartTime() const { return mistStartTime; }

    // Configuration
    static const unsigned long MIST_DURATION = 25000;      // 25 seconds
    static const unsigned long MIST_INTERVAL = 7200000;    // 2 hours
    static const int ACTIVE_WINDOW_START = 9;              // 9am
    static const int ACTIVE_WINDOW_END = 18;               // 6pm (exclusive)

private:
    ITimeProvider* timeProvider;
    IRelayController* relayController;

    MisterState currentState;
    unsigned long lastMistTime;
    unsigned long mistStartTime;

    // Internal logic methods
    bool isInActiveWindow();
    bool shouldStartMisting();
    void startMisting();
    void stopMisting();
};

#endif
```

**Design decisions:**
- Constructor takes pointers to interfaces (dependency injection)
- `update()` is the main entry point called from loop
- State and timing exposed via getters for testing/monitoring
- Configuration as public constants (easily testable)
- All logic is private, tested through the public `update()` method

## On-Device Implementations

### NTPTimeProvider

```cpp
// NTPTimeProvider.h
#ifndef NTP_TIME_PROVIDER_H
#define NTP_TIME_PROVIDER_H

#include "ITimeProvider.h"
#include <Arduino.h>

class NTPTimeProvider : public ITimeProvider {
public:
    bool getTime(struct tm* timeinfo) override {
        return getLocalTime(timeinfo);
    }

    unsigned long getMillis() override {
        return millis();
    }
};

#endif
```

### GPIORelayController

```cpp
// GPIORelayController.h
#ifndef GPIO_RELAY_CONTROLLER_H
#define GPIO_RELAY_CONTROLLER_H

#include "IRelayController.h"
#include <Arduino.h>

class GPIORelayController : public IRelayController {
public:
    GPIORelayController(int pin) : relayPin(pin) {
        pinMode(relayPin, OUTPUT);
        digitalWrite(relayPin, LOW);  // Safety: OFF on construction
    }

    void turnOn() override {
        digitalWrite(relayPin, HIGH);
    }

    void turnOff() override {
        digitalWrite(relayPin, LOW);
    }

private:
    int relayPin;
};

#endif
```

**Design rationale:**
- Thin wrappers around Arduino/ESP32 functions
- All hardware dependencies isolated here
- `GPIORelayController` handles pin setup and safety default in constructor
- Header-only implementations (simple enough, no `.cpp` needed)

## Test Mocks

### MockTimeProvider

```cpp
// test/native/mocks/MockTimeProvider.h
#ifndef MOCK_TIME_PROVIDER_H
#define MOCK_TIME_PROVIDER_H

#include "ITimeProvider.h"
#include <cstring>

class MockTimeProvider : public ITimeProvider {
public:
    MockTimeProvider() : timeAvailable(true), currentMillis(0) {
        memset(&mockTime, 0, sizeof(mockTime));
        // Default to a valid time
        mockTime.tm_year = 126;  // 2026
        mockTime.tm_mon = 0;     // January
        mockTime.tm_mday = 22;
        mockTime.tm_hour = 10;   // 10am (in active window)
        mockTime.tm_min = 0;
        mockTime.tm_sec = 0;
    }

    bool getTime(struct tm* timeinfo) override {
        if (!timeAvailable) return false;
        *timeinfo = mockTime;
        return true;
    }

    unsigned long getMillis() override {
        return currentMillis;
    }

    // Test control methods
    void setHour(int hour) { mockTime.tm_hour = hour; }
    void setTimeAvailable(bool available) { timeAvailable = available; }
    void advanceMillis(unsigned long ms) { currentMillis += ms; }
    void setMillis(unsigned long ms) { currentMillis = ms; }

private:
    struct tm mockTime;
    bool timeAvailable;
    unsigned long currentMillis;
};

#endif
```

### MockRelayController

```cpp
// test/native/mocks/MockRelayController.h
#ifndef MOCK_RELAY_CONTROLLER_H
#define MOCK_RELAY_CONTROLLER_H

#include "IRelayController.h"

class MockRelayController : public IRelayController {
public:
    MockRelayController() : isOn(false), turnOnCount(0), turnOffCount(0) {}

    void turnOn() override {
        isOn = true;
        turnOnCount++;
    }

    void turnOff() override {
        isOn = false;
        turnOffCount++;
    }

    // Test inspection methods
    bool getIsOn() const { return isOn; }
    int getTurnOnCount() const { return turnOnCount; }
    int getTurnOffCount() const { return turnOffCount; }
    void reset() { isOn = false; turnOnCount = 0; turnOffCount = 0; }

private:
    bool isOn;
    int turnOnCount;
    int turnOffCount;
};

#endif
```

**Design rationale:**
- Full control over time for testing different scenarios
- Track relay state changes for verification
- Simple, no dependencies on mocking frameworks
- Easy to set up test scenarios (e.g., "set time to 8am", "advance 2 hours")

## Native Test Structure

### Test Organization

1. **test_time_window.cpp** - Tests time window enforcement
   - Test hour 8 (before window) → no misting
   - Test hour 9 (start of window) → allows misting
   - Test hour 17 (end of window) → allows misting
   - Test hour 18 (after window) → no misting
   - Test when time unavailable → no misting

2. **test_state_machine.cpp** - Tests state transitions
   - WAITING_SYNC → IDLE when time becomes available
   - IDLE → MISTING when conditions met
   - MISTING → IDLE after 25 seconds
   - State stays WAITING_SYNC when time unavailable
   - State stays IDLE when outside window
   - State stays IDLE when interval not elapsed

3. **test_interval_timing.cpp** - Tests 2-hour interval logic
   - First mist triggers immediately (lastMistTime = 0)
   - Second mist waits 2 hours
   - Mist before 2 hours → stays IDLE
   - Mist at exactly 2 hours → triggers
   - Multiple cycles maintain 2-hour spacing

### Example Test Pattern

```cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "mocks/MockTimeProvider.h"
#include "mocks/MockRelayController.h"

void test_misting_blocked_before_9am() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(8);  // 8am - before window
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_misting_blocked_before_9am);
    // ... more tests
    return UNITY_END();
}
```

**Test Framework:** PlatformIO's built-in Unity test framework (simple, no external dependencies)

## On-Device Integration Tests

### test/embedded/test_timing_accuracy.cpp

Tests that verify real hardware timing and behavior:

1. **Relay activation duration test**
   - Trigger misting, measure actual relay-on time
   - Verify it's 25 seconds ±500ms tolerance
   - Check relay returns to OFF state

2. **State transition timing test**
   - Verify IDLE → MISTING transition happens promptly
   - Verify MISTING → IDLE happens at correct time
   - Measure loop execution time (should be <100ms)

3. **Time window boundary test**
   - Set system time near 9am boundary, verify misting starts
   - Set system time near 6pm boundary, verify misting allowed
   - Advance time past 6pm during misting, verify completes normally

4. **NTP time integration test**
   - Verify scheduler waits for NTP sync (WAITING_SYNC state)
   - Verify transitions to IDLE after sync
   - Verify scheduling works with real NTP time

**Test Approach:**
- These run on actual ESP32 hardware
- Use real NTP, real GPIO, real millis()
- Similar format to existing `test/test_main.ino`
- Report pass/fail with serial output

**Key Difference:**
- Native tests verify *logic correctness* (fast, no hardware)
- Embedded tests verify *timing accuracy and hardware integration* (slower, requires device)

## PlatformIO Configuration

### platformio.ini

```ini
[platformio]
default_envs = esp32

[env:esp32]
platform = espressif32
board = adafruit_feather_esp32s2
framework = arduino

; Source code location
src_dir = src

; Library dependencies
lib_deps =
    adafruit/Adafruit TestBed

; Build flags
build_flags =
    -DCORE_DEBUG_LEVEL=3

; Serial monitor
monitor_speed = 115200

[env:native]
platform = native
test_framework = unity

; Only build test code for native
test_build_src = yes

; Include src in build for native tests
build_flags =
    -std=c++11
    -I src/
```

## Migration Strategy

### Transition Approach

1. **Keep existing files during transition:**
   - `stevebot.ino` remains temporarily
   - New `src/main.cpp` created with refactored code
   - Both can coexist during development

2. **src/main.cpp structure:**

```cpp
#include <Arduino.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"

#define RELAY_PIN 13

// Global instances
NTPTimeProvider timeProvider;
GPIORelayController relayController(RELAY_PIN);
MistingScheduler scheduler(&timeProvider, &relayController);

void setup() {
    Serial.begin(115200);

    // WiFi and NTP setup (same as before)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org");
    }
}

void loop() {
    scheduler.update();
    delay(100);
}
```

3. **Logging integration:**
   - Add logging callback to `MistingScheduler` constructor
   - Or use observer pattern for state change notifications
   - Keeps scheduler pure, allows flexible logging

## Build Commands

```bash
# Build and upload main firmware
pio run -e esp32 -t upload

# Run native tests (on computer)
pio test -e native

# Run embedded tests (on device)
pio test -e esp32

# Monitor serial output
pio device monitor
```

## Testing Strategy

### Development Workflow

1. **Write native test** - Fast feedback on logic
2. **Implement feature** - Make native test pass
3. **Run embedded test** - Verify on hardware
4. **Iterate** - Fix any timing or integration issues

### Test Coverage Goals

- **Native tests:** 100% coverage of scheduling logic
- **Embedded tests:** Critical timing and integration paths
- **Manual tests:** End-to-end system behavior over 24 hours

## Benefits

1. **Fast TDD cycle** - Native tests run in milliseconds
2. **Comprehensive coverage** - All logic paths tested
3. **Hardware validation** - Integration tests catch timing issues
4. **Future-proof** - Clean architecture supports HTTP server addition
5. **Maintainable** - Clear separation of concerns
6. **Arduino compatible** - PlatformIO works with existing workflow

## Migration Checklist

- [ ] Install PlatformIO
- [ ] Create `platformio.ini`
- [ ] Define interfaces (ITimeProvider, IRelayController)
- [ ] Implement on-device classes (NTPTimeProvider, GPIORelayController)
- [ ] Extract MistingScheduler class
- [ ] Create test mocks
- [ ] Write native unit tests
- [ ] Write embedded integration tests
- [ ] Create src/main.cpp
- [ ] Verify native tests pass
- [ ] Verify embedded tests pass on hardware
- [ ] Remove stevebot.ino (optional)
- [ ] Update documentation

## Future Enhancements

This architecture enables:
- HTTP server can observe scheduler state via getters
- Manual override by calling scheduler methods from HTTP handlers
- Additional schedulers (lighting, heating) with same pattern
- Data logging and analytics
- Remote monitoring and control
