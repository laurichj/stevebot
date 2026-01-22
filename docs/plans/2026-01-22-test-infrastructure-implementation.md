# Test Infrastructure Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Refactor misting scheduler into testable components with comprehensive native unit tests and embedded integration tests using PlatformIO.

**Architecture:** Extract scheduling logic into `MistingScheduler` class with dependency injection (ITimeProvider, IRelayController interfaces). Native tests use mocks for fast feedback. Embedded tests verify timing on real hardware.

**Tech Stack:** PlatformIO, Unity test framework, ESP32 Arduino, C++11

---

## Task 1: Install and Initialize PlatformIO

**Files:**
- Create: `platformio.ini`

**Step 1: Install PlatformIO**

Run:
```bash
pip install platformio
```

Expected: PlatformIO CLI installed

**Step 2: Verify installation**

Run:
```bash
pio --version
```

Expected: Version number displayed (e.g., "PlatformIO Core, version 6.x.x")

**Step 3: Create platformio.ini**

Create file at project root:

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

**Step 4: Test PlatformIO setup**

Run:
```bash
pio run -e esp32 --dry-run
```

Expected: No errors, shows it would compile for ESP32

**Step 5: Commit**

```bash
git add platformio.ini
git commit -m "Add PlatformIO configuration with ESP32 and native test environments"
```

---

## Task 2: Create Interface Headers

**Files:**
- Create: `src/ITimeProvider.h`
- Create: `src/IRelayController.h`

**Step 1: Create src directory**

Run:
```bash
mkdir -p src
```

**Step 2: Create ITimeProvider.h**

```cpp
// src/ITimeProvider.h
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

**Step 3: Create IRelayController.h**

```cpp
// src/IRelayController.h
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

**Step 4: Verify compilation**

Run:
```bash
pio run -e esp32 --dry-run
```

Expected: No errors (headers don't produce compilation yet, just syntax check)

**Step 5: Commit**

```bash
git add src/ITimeProvider.h src/IRelayController.h
git commit -m "Add time provider and relay controller interfaces"
```

---

## Task 3: Create MistingScheduler Header

**Files:**
- Create: `src/MistingScheduler.h`

**Step 1: Create MistingScheduler.h**

```cpp
// src/MistingScheduler.h
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

**Step 2: Commit**

```bash
git add src/MistingScheduler.h
git commit -m "Add MistingScheduler class header"
```

---

## Task 4: Create Test Mocks

**Files:**
- Create: `test/native/mocks/MockTimeProvider.h`
- Create: `test/native/mocks/MockRelayController.h`

**Step 1: Create test directory structure**

Run:
```bash
mkdir -p test/native/mocks
```

**Step 2: Create MockTimeProvider.h**

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

**Step 3: Create MockRelayController.h**

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

**Step 4: Commit**

```bash
git add test/native/mocks/
git commit -m "Add mock implementations for native testing"
```

---

## Task 5: Write First Test (Time Window - Before 9am)

**Files:**
- Create: `test/native/test_time_window.cpp`

**Step 1: Write failing test**

```cpp
// test/native/test_time_window.cpp
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
    return UNITY_END();
}
```

**Step 2: Run test to verify it fails**

Run:
```bash
pio test -e native
```

Expected: FAIL - MistingScheduler not implemented yet

**Step 3: Commit**

```bash
git add test/native/test_time_window.cpp
git commit -m "Add failing test for time window enforcement (before 9am)"
```

---

## Task 6: Implement MistingScheduler (Minimal to Pass First Test)

**Files:**
- Create: `src/MistingScheduler.cpp`

**Step 1: Create MistingScheduler.cpp with constructor**

```cpp
// src/MistingScheduler.cpp
#include "MistingScheduler.h"

MistingScheduler::MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController)
    : timeProvider(timeProvider), relayController(relayController),
      currentState(WAITING_SYNC), lastMistTime(0), mistStartTime(0) {
}

void MistingScheduler::update() {
    switch (currentState) {
        case WAITING_SYNC:
            // Assume time is available for now (will refine later)
            currentState = IDLE;
            lastMistTime = 0;
            break;

        case IDLE:
            if (shouldStartMisting()) {
                startMisting();
            }
            break;

        case MISTING:
            unsigned long elapsed = timeProvider->getMillis() - mistStartTime;
            if (elapsed >= MIST_DURATION) {
                stopMisting();
            }
            break;
    }
}

bool MistingScheduler::isInActiveWindow() {
    struct tm timeinfo;
    if (!timeProvider->getTime(&timeinfo)) {
        return false;
    }

    int hour = timeinfo.tm_hour;
    return (hour >= ACTIVE_WINDOW_START && hour < ACTIVE_WINDOW_END);
}

bool MistingScheduler::shouldStartMisting() {
    if (!isInActiveWindow()) return false;
    if (currentState != IDLE) return false;

    // First mist
    if (lastMistTime == 0) return true;

    // Check if 2 hours have passed
    unsigned long elapsed = timeProvider->getMillis() - lastMistTime;
    return (elapsed >= MIST_INTERVAL);
}

void MistingScheduler::startMisting() {
    relayController->turnOn();
    mistStartTime = timeProvider->getMillis();
    lastMistTime = timeProvider->getMillis();
    currentState = MISTING;
}

void MistingScheduler::stopMisting() {
    relayController->turnOff();
    currentState = IDLE;
}
```

**Step 2: Run test to verify it passes**

Run:
```bash
pio test -e native
```

Expected: PASS - 1 test passing

**Step 3: Commit**

```bash
git add src/MistingScheduler.cpp
git commit -m "Implement MistingScheduler with time window enforcement"
```

---

## Task 7: Add More Time Window Tests

**Files:**
- Modify: `test/native/test_time_window.cpp`

**Step 1: Add tests for window boundaries**

Add these tests to `test_time_window.cpp` before `main()`:

```cpp
void test_misting_allowed_at_9am() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(9);  // 9am - start of window
    scheduler.update();

    TEST_ASSERT_TRUE(relay.getIsOn());
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_misting_allowed_at_5pm() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(17);  // 5pm - end of window
    scheduler.update();

    TEST_ASSERT_TRUE(relay.getIsOn());
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_misting_blocked_at_6pm() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(18);  // 6pm - after window
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_misting_blocked_when_time_unavailable() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setTimeAvailable(false);
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}
```

**Step 2: Update main() to run new tests**

Replace the `main()` function:

```cpp
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_misting_blocked_before_9am);
    RUN_TEST(test_misting_allowed_at_9am);
    RUN_TEST(test_misting_allowed_at_5pm);
    RUN_TEST(test_misting_blocked_at_6pm);
    RUN_TEST(test_misting_blocked_when_time_unavailable);
    return UNITY_END();
}
```

**Step 3: Run tests**

Run:
```bash
pio test -e native
```

Expected: PASS - 5 tests passing

**Step 4: Commit**

```bash
git add test/native/test_time_window.cpp
git commit -m "Add comprehensive time window boundary tests"
```

---

## Task 8: Add State Machine Tests

**Files:**
- Create: `test/native/test_state_machine.cpp`

**Step 1: Write state machine tests**

```cpp
// test/native/test_state_machine.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "mocks/MockTimeProvider.h"
#include "mocks/MockRelayController.h"

void test_initial_state_is_waiting_sync() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    // Mock time as unavailable initially
    timeProvider.setTimeAvailable(false);

    MistingScheduler scheduler(&timeProvider, &relay);

    // Don't call update - check initial state
    TEST_ASSERT_EQUAL(WAITING_SYNC, scheduler.getState());
}

void test_transitions_to_idle_when_time_available() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    timeProvider.setTimeAvailable(false);
    MistingScheduler scheduler(&timeProvider, &relay);

    TEST_ASSERT_EQUAL(WAITING_SYNC, scheduler.getState());

    // Make time available
    timeProvider.setTimeAvailable(true);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_transitions_to_misting_when_conditions_met() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);  // In window
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_TRUE(relay.getIsOn());
}

void test_transitions_to_idle_after_25_seconds() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    scheduler.update();  // Start misting

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());

    // Advance time by 25 seconds
    timeProvider.advanceMillis(25000);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
}

void test_stays_idle_when_outside_window() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(8);  // Before window
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Multiple updates shouldn't change state
    scheduler.update();
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_waiting_sync);
    RUN_TEST(test_transitions_to_idle_when_time_available);
    RUN_TEST(test_transitions_to_misting_when_conditions_met);
    RUN_TEST(test_transitions_to_idle_after_25_seconds);
    RUN_TEST(test_stays_idle_when_outside_window);
    return UNITY_END();
}
```

**Step 2: Run tests**

Run:
```bash
pio test -e native
```

Expected: FAIL - `test_initial_state_is_waiting_sync` fails because constructor currently transitions to IDLE immediately

**Step 3: Fix MistingScheduler to check time availability**

Modify `MistingScheduler.cpp` constructor and update method:

In constructor, keep `currentState(WAITING_SYNC)` initialization.

In `update()` WAITING_SYNC case, change to:

```cpp
case WAITING_SYNC:
    {
        struct tm timeinfo;
        if (timeProvider->getTime(&timeinfo)) {
            currentState = IDLE;
            lastMistTime = 0;
        }
    }
    break;
```

**Step 4: Run tests again**

Run:
```bash
pio test -e native
```

Expected: PASS - All state machine tests passing

**Step 5: Commit**

```bash
git add test/native/test_state_machine.cpp src/MistingScheduler.cpp
git commit -m "Add state machine tests and fix time sync checking"
```

---

## Task 9: Add Interval Timing Tests

**Files:**
- Create: `test/native/test_interval_timing.cpp`

**Step 1: Write interval timing tests**

```cpp
// test/native/test_interval_timing.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "mocks/MockTimeProvider.h"
#include "mocks/MockRelayController.h"

void test_first_mist_triggers_immediately() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);  // In window
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());
}

void test_second_mist_waits_2_hours() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    scheduler.update();  // First mist starts

    // Complete first mist
    timeProvider.advanceMillis(25000);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Try to mist before 2 hours
    timeProvider.advanceMillis(3600000);  // 1 hour
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());  // Still only 1
}

void test_mist_triggers_at_2_hour_mark() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    scheduler.update();  // First mist

    // Complete first mist
    timeProvider.advanceMillis(25000);
    scheduler.update();

    // Advance exactly 2 hours from first mist start
    timeProvider.setMillis(7200000);  // 2 hours total
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_EQUAL(2, relay.getTurnOnCount());
}

void test_multiple_cycles_maintain_spacing() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);

    // First cycle
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());

    // Second cycle (2 hours later)
    timeProvider.setMillis(7200000);
    timeProvider.setHour(12);
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(2, relay.getTurnOnCount());

    // Third cycle (4 hours from start)
    timeProvider.setMillis(14400000);
    timeProvider.setHour(14);
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(3, relay.getTurnOnCount());
}

void test_mist_blocked_before_interval_even_in_window() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    scheduler.update();  // First mist

    timeProvider.advanceMillis(25000);
    scheduler.update();  // Complete first mist

    // Still in window but only 30 minutes later
    timeProvider.setMillis(25000 + 1800000);  // +30 min
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_first_mist_triggers_immediately);
    RUN_TEST(test_second_mist_waits_2_hours);
    RUN_TEST(test_mist_triggers_at_2_hour_mark);
    RUN_TEST(test_multiple_cycles_maintain_spacing);
    RUN_TEST(test_mist_blocked_before_interval_even_in_window);
    return UNITY_END();
}
```

**Step 2: Run tests**

Run:
```bash
pio test -e native
```

Expected: PASS - All interval timing tests should pass with existing implementation

**Step 3: Commit**

```bash
git add test/native/test_interval_timing.cpp
git commit -m "Add interval timing tests for 2-hour misting cycles"
```

---

## Task 10: Create On-Device Implementations

**Files:**
- Create: `src/NTPTimeProvider.h`
- Create: `src/GPIORelayController.h`

**Step 1: Create NTPTimeProvider.h**

```cpp
// src/NTPTimeProvider.h
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

**Step 2: Create GPIORelayController.h**

```cpp
// src/GPIORelayController.h
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

**Step 3: Verify compilation for ESP32**

Run:
```bash
pio run -e esp32
```

Expected: SUCCESS - Compiles for ESP32

**Step 4: Commit**

```bash
git add src/NTPTimeProvider.h src/GPIORelayController.h
git commit -m "Add on-device implementations for time and relay control"
```

---

## Task 11: Create Refactored Main Entry Point

**Files:**
- Create: `src/main.cpp`

**Step 1: Create main.cpp with full WiFi and scheduler setup**

```cpp
// src/main.cpp
#include <Arduino.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"

#define RELAY_PIN 13

// NTP server configuration
const char* ntpServer = "pool.ntp.org";

// Global instances
NTPTimeProvider timeProvider;
GPIORelayController relayController(RELAY_PIN);
MistingScheduler scheduler(&timeProvider, &relayController);

void setup() {
    Serial.begin(115200);

    // WiFi setup
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
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
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Initialize NTP time synchronization
        Serial.println("Synchronizing time with NTP server...");
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);

        // Wait for time to be set
        struct tm timeinfo;
        int ntpAttempts = 0;
        while (!getLocalTime(&timeinfo) && ntpAttempts < 10) {
            Serial.print(".");
            delay(500);
            ntpAttempts++;
        }

        if (getLocalTime(&timeinfo)) {
            Serial.println("\nTime synchronized!");
            Serial.print("Current time: ");
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        } else {
            Serial.println("\nFailed to synchronize time!");
        }
    } else {
        Serial.println("\nWiFi connection failed!");
    }
}

void loop() {
    scheduler.update();
    delay(100);
}
```

**Step 2: Build for ESP32**

Run:
```bash
pio run -e esp32
```

Expected: SUCCESS - Firmware builds successfully

**Step 3: Commit**

```bash
git add src/main.cpp
git commit -m "Create refactored main entry point with scheduler integration"
```

---

## Task 12: Add Logging to MistingScheduler

**Files:**
- Modify: `src/MistingScheduler.h`
- Modify: `src/MistingScheduler.cpp`

**Step 1: Add logging callback to header**

In `MistingScheduler.h`, add after includes:

```cpp
// Logging callback type
typedef void (*LogCallback)(const char* message);
```

Update constructor signature:

```cpp
MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, LogCallback logger = nullptr);
```

Add private member:

```cpp
private:
    LogCallback logger;
```

**Step 2: Update implementation to use logger**

In `MistingScheduler.cpp`, update constructor:

```cpp
MistingScheduler::MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, LogCallback logger)
    : timeProvider(timeProvider), relayController(relayController), logger(logger),
      currentState(WAITING_SYNC), lastMistTime(0), mistStartTime(0) {
}
```

Add logging helper:

```cpp
void MistingScheduler::log(const char* message) {
    if (logger) {
        logger(message);
    }
}
```

Add to header as private method:

```cpp
void log(const char* message);
```

Update `startMisting()`:

```cpp
void MistingScheduler::startMisting() {
    relayController->turnOn();
    mistStartTime = timeProvider->getMillis();
    lastMistTime = timeProvider->getMillis();
    currentState = MISTING;
    log("MIST START");
}
```

Update `stopMisting()`:

```cpp
void MistingScheduler::stopMisting() {
    relayController->turnOff();
    currentState = IDLE;
    log("MIST STOP");
}
```

**Step 3: Update main.cpp to provide logger**

In `src/main.cpp`, add logging function before setup:

```cpp
void logWithTimestamp(const char* message) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.printf("%04d-%02d-%02d %02d:%02d:%02d | %s\n",
                      timeinfo.tm_year + 1900,
                      timeinfo.tm_mon + 1,
                      timeinfo.tm_mday,
                      timeinfo.tm_hour,
                      timeinfo.tm_min,
                      timeinfo.tm_sec,
                      message);
    } else {
        Serial.printf("----/--/-- --:--:-- | %s\n", message);
    }
}
```

Update scheduler instantiation:

```cpp
MistingScheduler scheduler(&timeProvider, &relayController, logWithTimestamp);
```

**Step 4: Build and verify**

Run:
```bash
pio run -e esp32
```

Expected: SUCCESS

**Step 5: Run native tests to ensure still passing**

Run:
```bash
pio test -e native
```

Expected: PASS - All tests still pass (logger is optional, tests pass nullptr)

**Step 6: Commit**

```bash
git add src/MistingScheduler.h src/MistingScheduler.cpp src/main.cpp
git commit -m "Add logging callback support to MistingScheduler"
```

---

## Task 13: Create Embedded Integration Test

**Files:**
- Create: `test/embedded/test_timing_accuracy.cpp`

**Step 1: Create embedded test directory**

Run:
```bash
mkdir -p test/embedded
```

**Step 2: Create test_timing_accuracy.cpp**

```cpp
// test/embedded/test_timing_accuracy.cpp
#include <Arduino.h>
#include <unity.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"

#define RELAY_PIN 13
#define TOLERANCE_MS 500

NTPTimeProvider* timeProvider;
GPIORelayController* relayController;
MistingScheduler* scheduler;

void setUp(void) {
    // Runs before each test
}

void tearDown(void) {
    // Runs after each test
}

void test_relay_activation_duration() {
    Serial.println("Testing relay activation duration...");

    // Force time to be in window
    struct tm timeinfo;
    timeProvider->getTime(&timeinfo);
    timeinfo.tm_hour = 10;  // Force 10am

    unsigned long startTime = millis();

    // Trigger misting by simulating first boot
    scheduler->update();

    TEST_ASSERT_EQUAL(MISTING, scheduler->getState());

    // Wait for misting to complete
    while (scheduler->getState() == MISTING) {
        scheduler->update();
        delay(100);
    }

    unsigned long duration = millis() - startTime;

    Serial.printf("Misting duration: %lu ms\n", duration);

    // Should be 25000ms ± 500ms
    TEST_ASSERT_INT_WITHIN(TOLERANCE_MS, 25000, duration);
}

void test_state_transition_timing() {
    Serial.println("Testing state transition timing...");

    unsigned long loopStart = millis();
    scheduler->update();
    unsigned long loopDuration = millis() - loopStart;

    // Loop should execute quickly (< 100ms)
    TEST_ASSERT_LESS_THAN(100, loopDuration);
}

void test_ntp_integration() {
    Serial.println("Testing NTP integration...");

    struct tm timeinfo;
    bool timeAvailable = timeProvider->getTime(&timeinfo);

    TEST_ASSERT_TRUE(timeAvailable);
    TEST_ASSERT_GREATER_THAN(2020, timeinfo.tm_year + 1900);
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  STEVEBOT TIMING ACCURACY TESTS");
    Serial.println("========================================\n");

    // Setup WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");

        // Setup NTP
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org");
        struct tm timeinfo;
        int ntpAttempts = 0;
        while (!getLocalTime(&timeinfo) && ntpAttempts < 10) {
            delay(500);
            ntpAttempts++;
        }
    }

    // Create instances
    timeProvider = new NTPTimeProvider();
    relayController = new GPIORelayController(RELAY_PIN);
    scheduler = new MistingScheduler(timeProvider, relayController);

    // Run tests
    UNITY_BEGIN();
    RUN_TEST(test_ntp_integration);
    RUN_TEST(test_state_transition_timing);
    RUN_TEST(test_relay_activation_duration);
    UNITY_END();
}

void loop() {
    // Tests run in setup()
}
```

**Step 3: Build embedded tests**

Run:
```bash
pio test -e esp32
```

Expected: Will need hardware connected to actually run, but should compile

**Step 4: Commit**

```bash
git add test/embedded/test_timing_accuracy.cpp
git commit -m "Add embedded integration tests for timing accuracy"
```

---

## Task 14: Update Documentation

**Files:**
- Modify: `README.md`
- Create: `docs/TESTING.md`

**Step 1: Update README.md**

Add after the "Running Tests" section:

```markdown
## Testing

Stevebot uses a hybrid testing approach with both native unit tests and embedded integration tests.

### Native Unit Tests (Fast)

Run on your computer without hardware:

```bash
pio test -e native
```

Tests include:
- Time window enforcement (9am-6pm)
- State machine transitions
- 2-hour interval timing logic

### Embedded Integration Tests (Hardware Required)

Run on actual ESP32 hardware:

```bash
pio test -e esp32
```

Tests include:
- Relay activation timing accuracy
- NTP time synchronization
- Real-world state transitions

See [docs/TESTING.md](docs/TESTING.md) for detailed testing guide.
```

**Step 2: Create TESTING.md**

```markdown
# Testing Guide

## Overview

Stevebot uses a hybrid testing strategy:
- **Native tests** verify logic correctness (fast, no hardware needed)
- **Embedded tests** verify timing accuracy and hardware integration

## Architecture

### Test Structure

```
test/
├── native/                    # Native unit tests
│   ├── test_time_window.cpp
│   ├── test_state_machine.cpp
│   ├── test_interval_timing.cpp
│   └── mocks/
│       ├── MockTimeProvider.h
│       └── MockRelayController.h
└── embedded/                  # Embedded integration tests
    └── test_timing_accuracy.cpp
```

### Dependency Injection

The `MistingScheduler` class uses dependency injection for testability:

- `ITimeProvider` interface - Abstracts time access
- `IRelayController` interface - Abstracts relay control

**On device:** Real implementations (NTPTimeProvider, GPIORelayController)
**In tests:** Mocks with full control over behavior

## Running Tests

### Native Tests

```bash
# Run all native tests
pio test -e native

# Run specific test file
pio test -e native -f test_time_window

# Verbose output
pio test -e native -v
```

**Advantages:**
- Instant feedback (milliseconds)
- No hardware required
- Easy to test edge cases
- Run in CI/CD pipelines

### Embedded Tests

```bash
# Upload and run on device
pio test -e esp32

# Specify port
pio test -e esp32 --upload-port /dev/ttyUSB0
```

**Requirements:**
- ESP32 connected via USB
- Valid secrets.h with WiFi credentials
- Network connectivity for NTP tests

## Test Coverage

### Time Window Tests (`test_time_window.cpp`)

- ✅ Misting blocked before 9am
- ✅ Misting allowed at 9am
- ✅ Misting allowed at 5pm
- ✅ Misting blocked at 6pm
- ✅ Misting blocked when time unavailable

### State Machine Tests (`test_state_machine.cpp`)

- ✅ Initial state is WAITING_SYNC
- ✅ Transitions to IDLE when time available
- ✅ Transitions to MISTING when conditions met
- ✅ Transitions to IDLE after 25 seconds
- ✅ Stays IDLE when outside window

### Interval Timing Tests (`test_interval_timing.cpp`)

- ✅ First mist triggers immediately
- ✅ Second mist waits 2 hours
- ✅ Mist triggers at 2-hour mark exactly
- ✅ Multiple cycles maintain spacing
- ✅ Mist blocked before interval

### Embedded Integration Tests (`test_timing_accuracy.cpp`)

- ✅ Relay activation duration (25s ±500ms)
- ✅ State transition timing (<100ms per loop)
- ✅ NTP integration with real time

## Writing New Tests

### Native Test Template

```cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "mocks/MockTimeProvider.h"
#include "mocks/MockRelayController.h"

void test_my_feature() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    // Setup test conditions
    timeProvider.setHour(10);

    // Execute
    scheduler.update();

    // Assert
    TEST_ASSERT_EQUAL(expected, actual);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_my_feature);
    return UNITY_END();
}
```

### Mock Control Methods

**MockTimeProvider:**
- `setHour(int hour)` - Set time to specific hour
- `setTimeAvailable(bool)` - Simulate time sync failure
- `advanceMillis(unsigned long)` - Advance time
- `setMillis(unsigned long)` - Set absolute time

**MockRelayController:**
- `getIsOn()` - Check relay state
- `getTurnOnCount()` - Count activations
- `getTurnOffCount()` - Count deactivations
- `reset()` - Reset state for next test

## Continuous Integration

Native tests can run in CI without hardware:

```yaml
# Example GitHub Actions
- name: Run Tests
  run: |
    pip install platformio
    pio test -e native
```

## Troubleshooting

### Native Tests Fail to Compile

- Check that `platformio.ini` has correct paths
- Ensure `test_build_src = yes` in `[env:native]`
- Verify mocks are in `test/native/mocks/`

### Embedded Tests Timeout

- Verify WiFi credentials in `secrets.h`
- Check USB connection and port
- Ensure NTP traffic allowed on network
- Try increasing timeout in test code

### Test Results Not Appearing

- Check serial monitor baud rate (115200)
- Use `pio test -e esp32 -v` for verbose output
- Ensure `monitor_speed = 115200` in platformio.ini
```

**Step 3: Commit**

```bash
git add README.md docs/TESTING.md
git commit -m "Update documentation with testing guide"
```

---

## Task 15: Final Verification and Cleanup

**Files:**
- Test: All native tests
- Test: Build for ESP32
- Optional: Remove `stevebot.ino` if migrating fully

**Step 1: Run all native tests**

Run:
```bash
pio test -e native -v
```

Expected: All tests pass (15+ tests across 3 files)

**Step 2: Build firmware for ESP32**

Run:
```bash
pio run -e esp32
```

Expected: SUCCESS - Firmware builds without errors

**Step 3: Optional - Test on hardware**

If hardware available:
```bash
pio run -e esp32 -t upload
pio device monitor
```

Watch for:
- WiFi connection
- NTP sync
- Scheduler state transitions
- Misting cycles (if in active window)

**Step 4: Optional - Remove old stevebot.ino**

If migrating fully to PlatformIO:
```bash
git rm stevebot.ino
git commit -m "Remove legacy Arduino sketch (migrated to PlatformIO)"
```

Or keep both during transition period.

**Step 5: Final commit and summary**

Create summary of what was built:

```bash
git log --oneline --no-decorate | head -20 > MIGRATION_SUMMARY.txt
git add MIGRATION_SUMMARY.txt
git commit -m "Add migration summary for test infrastructure implementation"
```

---

## Completion Checklist

- [ ] PlatformIO installed and configured
- [ ] Interfaces defined (ITimeProvider, IRelayController)
- [ ] MistingScheduler class implemented
- [ ] Mock implementations created
- [ ] Time window tests passing (5 tests)
- [ ] State machine tests passing (5 tests)
- [ ] Interval timing tests passing (5 tests)
- [ ] On-device implementations created
- [ ] Main.cpp refactored with scheduler
- [ ] Logging integrated
- [ ] Embedded integration tests created
- [ ] Documentation updated
- [ ] All native tests passing
- [ ] ESP32 firmware builds successfully
- [ ] Optional: Tested on hardware
- [ ] Optional: Legacy code removed

## Next Steps

1. **Use @superpowers:verification-before-completion** to verify all functionality
2. **Test on hardware** if not done during implementation
3. **Use @superpowers:finishing-a-development-branch** to decide merge/PR strategy
4. Consider next features:
   - HTTP server with state monitoring
   - Manual override endpoints
   - EEPROM persistence for last mist time
   - Watchdog timer for safety
