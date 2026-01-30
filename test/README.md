# Stevebot Test Suite

This directory contains comprehensive unit and integration tests for the Stevebot environmental controller.

## Test Architecture

The project uses a **hybrid testing approach**:

- **Native Tests** (`test/test_*/`) - Fast unit tests using mock dependencies (no hardware required)
- **Embedded Tests** (`test/embedded/`) - Integration tests that run on actual ESP32 hardware

### Test Organization

```
test/
├── native/
│   └── mocks/
│       ├── MockTimeProvider.h         # Simulates ESP32 time functions
│       ├── MockRelayController.h      # Simulates relay hardware
│       └── MockStateStorage.h         # Simulates NVS storage
├── test_time_window/                  # Time window enforcement tests (5 tests)
├── test_state_machine/                # State machine transition tests (5 tests)
├── test_interval_timing/              # 2-hour interval tests (5 tests)
├── test_state_persistence/            # NVS save tests (4 tests)
├── test_state_recovery/               # NVS restore tests (6 tests)
├── test_scheduler_enable_disable/     # Enable/disable tests (4 tests)
├── test_force_mist/                   # Force mist command tests (4 tests)
├── test_mock_storage/                 # MockStateStorage verification (5 tests)
└── embedded/
    └── test_wifi_ntp.cpp              # WiFi and NTP integration tests
```

## Test Files Overview

### Native Unit Tests (38 tests total)

**Core Scheduler Tests:**
- `test_time_window/` - Validates 9am-6pm active window enforcement
- `test_state_machine/` - Tests state transitions (WAITING_SYNC → IDLE → MISTING)
- `test_interval_timing/` - Verifies 2-hour misting interval logic

**Safety Features Tests:**
- `test_state_persistence/` - Verifies state is saved to NVS after operations
- `test_state_recovery/` - Tests state restoration after power cycles
- `test_scheduler_enable_disable/` - Tests manual enable/disable functionality
- `test_force_mist/` - Tests manual force mist command and safety checks

**Mock Infrastructure Tests:**
- `test_mock_storage/` - Validates MockStateStorage test double behavior

## Running Tests

### Native Unit Tests (Recommended for Development)

Fast tests that run on your development machine without hardware:

```bash
# Run all native tests using Makefile
make test

# Run with verbose output
make test-verbose

# Run specific test
make test-specific TEST=test_state_machine

# Or use PlatformIO directly
pio test -e native
pio test -e native --filter test_state_machine
pio test -e native -v
```

**Advantages:**
- ⚡ Fast execution (~3-5 seconds for full suite)
- 🔄 No hardware required
- 🐛 Easy to debug
- 🎯 Perfect for TDD workflow

**Test Execution:**
```
Environment    Test                           Status    Duration
-------------  -----------------------------  --------  ------------
native         test_time_window               PASSED    00:00:00.553
native         test_state_machine             PASSED    00:00:00.499
native         test_interval_timing           PASSED    00:00:00.505
native         test_state_persistence         PASSED    00:00:00.485
native         test_state_recovery            PASSED    00:00:00.855
native         test_scheduler_enable_disable  PASSED    00:00:00.702
native         test_force_mist                PASSED    00:00:00.623
native         test_mock_storage              PASSED    00:00:00.495

================= 38 test cases: 38 succeeded in 00:00:04.718 =================
```

### Hardware Integration Testing

For testing WiFi and NTP functionality on actual ESP32 hardware, use the main firmware with the serial monitor:

```bash
# Build, upload, and monitor the main firmware
make flash

# Or use PlatformIO directly
pio run -e esp32 --target upload && pio device monitor -b 115200 --filter send_on_enter
```

The main firmware (`src/main.cpp`) includes comprehensive WiFi connection, NTP synchronization, and logging that serves as integration testing on hardware. Monitor the serial output to verify:

- WiFi connection and reconnection
- NTP time synchronization
- Misting scheduler state transitions
- Serial command interface (STATUS, ENABLE, DISABLE, FORCE_MIST)

**Note:** Hardware testing requires physical ESP32 V2 board and WiFi credentials configured in `src/secrets.h`.

## Mock Infrastructure

The native tests use **mock objects** to simulate hardware dependencies:

### MockTimeProvider
Simulates ESP32 time functions without requiring NTP sync:
```cpp
MockTimeProvider timeProvider;
timeProvider.setTime(9, 0, 0);  // Set time to 9:00 AM
timeProvider.advanceTime(7200000);  // Advance by 2 hours
```

### MockRelayController
Simulates relay hardware for testing on/off operations:
```cpp
MockRelayController relay;
// ... trigger misting ...
assert(relay.isOn() == true);  // Verify relay is on
```

### MockStateStorage
Simulates NVS (Non-Volatile Storage) for testing persistence:
```cpp
MockStateStorage storage;
storage.setLastMistTime(12345);  // Pre-populate state
// ... test state recovery ...
assert(storage.getSaveCallCount() == 1);  // Verify save was called
```

**Usage in Tests:**
All native tests inject these mocks into the `MistingScheduler` to test behavior without hardware dependencies.

## Test Coverage Details

### Native Unit Tests (38 tests)

#### Time Window Tests (5 tests)
- Misting blocked before 9am
- Misting allowed at 9am (start of window)
- Misting allowed at 5:59pm (end of window)
- Misting blocked at 6pm (outside window)
- Misting blocked when time unavailable

#### State Machine Tests (5 tests)
- Initial state is WAITING_SYNC
- Transitions to IDLE when time available
- Transitions to MISTING when conditions met
- Transitions to IDLE after 25 seconds
- Stays in IDLE when outside active window

#### Interval Timing Tests (5 tests)
- First mist triggers immediately
- Second mist waits 2 hours
- Mist triggers at 2-hour mark exactly
- Multiple cycles maintain 2-hour spacing
- Mist blocked before interval even in window

#### State Persistence Tests (4 tests)
- State saved after startMisting()
- State saved after stopMisting()
- State saved after setEnabled()
- No crash when storage is nullptr (backward compatibility)

#### State Recovery Tests (6 tests)
- loadState() restores lastMistTime
- loadState() restores hasEverMisted flag
- loadState() restores schedulerEnabled flag
- Handles nullptr storage gracefully
- Default values when storage empty
- Prevents immediate re-misting after recovery

#### Scheduler Enable/Disable Tests (4 tests)
- setEnabled(false) disables automatic misting
- setEnabled(true) enables automatic misting
- Disabled scheduler prevents scheduled mists
- Enabled flag persists to storage

#### Force Mist Tests (4 tests)
- forceMist() triggers immediate misting when enabled
- forceMist() blocked when already misting
- forceMist() blocked when scheduler disabled
- forceMist() updates lastMistTime

#### Mock Storage Tests (5 tests)
- Initialization defaults correct
- Save and retrieve operations work
- Multiple saves update values
- Setter methods update state
- Save call tracking accurate

## Writing New Tests

### Example Native Test

```cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "../native/mocks/MockTimeProvider.h"
#include "../native/mocks/MockRelayController.h"
#include "../native/mocks/MockStateStorage.h"

void test_my_new_feature() {
    // Arrange
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;
    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    timeProvider.setTime(9, 0, 0);  // 9am
    scheduler.loadState();

    // Act
    scheduler.update();

    // Assert
    TEST_ASSERT_TRUE(relay.isOn());
}

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_my_new_feature);
    return UNITY_END();
}
```

### Test Best Practices

1. **Use descriptive test names** - `test_forceMist_blocked_when_already_misting` is better than `test_force_mist_2`
2. **Follow AAA pattern** - Arrange (setup), Act (execute), Assert (verify)
3. **Test one thing per test** - Each test should verify a single behavior
4. **Use mocks appropriately** - Inject mock dependencies for isolated testing
5. **Clean state between tests** - Each test should be independent

## Troubleshooting

### Native Test Failures

**Issue:** Compilation errors about missing includes
- **Solution:** Ensure all mock headers are in `test/native/mocks/`
- **Solution:** Check that `platformio.ini` build flags exclude ESP32-specific code

**Issue:** Tests pass individually but fail when run together
- **Solution:** Ensure proper cleanup in `tearDown()` functions
- **Solution:** Check for static/global state that persists between tests

### Embedded Test Failures

**Issue:** WiFi connection failures
- Verify `secrets.h` has correct SSID and password
- Check that WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure WiFi network is in range

**Issue:** NTP synchronization failures
- Verify WiFi is connected first
- Check that network allows NTP traffic (UDP port 123)
- Try alternative NTP servers (edit `test_wifi_ntp.cpp`)
- Ensure router firewall isn't blocking NTP

**Issue:** Time accuracy problems
- Verify timezone offsets in `secrets.h` are correct
- Check for clock drift over longer periods
- Consider adjusting NTP server if geographically distant

## Related Documentation

- [Hardware Testing Guide](../docs/hardware-testing-guide.md) - Procedures for testing on actual hardware
- [Testing Infrastructure Design](../docs/plans/2026-01-22-test-infrastructure-design.md) - Architecture and design decisions
- [Safety Features Design](../docs/plans/2026-01-27-safety-features-design.md) - Safety feature specifications
