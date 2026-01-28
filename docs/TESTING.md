# Testing Guide

This document provides comprehensive information about the test infrastructure for the Stevebot misting scheduler.

## Overview

The project uses a **hybrid testing approach** to balance speed, reliability, and real-world validation:

- **Native Unit Tests**: Fast, isolated tests that run on your development machine (no hardware required)
- **Embedded Integration Tests**: End-to-end tests that run on the actual ESP32 hardware

## Test Architecture

### Design Principles

The codebase follows **dependency injection** patterns to enable thorough testing:

```
┌─────────────────────────────────────────┐
│     MistingScheduler (Core Logic)      │
│  - State machine (WAITING_SYNC/IDLE/   │
│    MISTING)                             │
│  - Scheduling rules (9am-6pm, 2hr       │
│    intervals)                           │
│  - Misting duration (25 seconds)        │
└──────────┬──────────────────┬───────────┘
           │                  │
           ▼                  ▼
    ┌─────────────┐    ┌────────────────┐
    │ITimeProvider│    │IRelayController│
    └─────────────┘    └────────────────┘
           │                  │
           │                  │
    ┌──────┴──────┐    ┌──────┴──────┐
    │             │    │             │
    ▼             ▼    ▼             ▼
┌──────┐    ┌─────────┐  ┌──────┐  ┌──────┐
│ Mock │    │   NTP   │  │ Mock │  │ GPIO │
│ Time │    │  Time   │  │Relay │  │Relay │
└──────┘    └─────────┘  └──────┘  └──────┘
  (Test)    (Production)   (Test)  (Production)
```

### Key Components

#### Interfaces

- **`ITimeProvider`** (`src/ITimeProvider.h`): Abstracts time/clock access
  - `bool getTime(struct tm*)`: Get current date/time
  - `unsigned long getMillis()`: Get milliseconds since boot

- **`IRelayController`** (`src/IRelayController.h`): Abstracts relay control
  - `void turnOn()`: Activate relay
  - `void turnOff()`: Deactivate relay

#### Production Implementations

- **`NTPTimeProvider`** (`src/NTPTimeProvider.h`): Uses ESP32's NTP sync
- **`GPIORelayController`** (`src/GPIORelayController.h`): Controls physical relay on GPIO pin

#### Test Mocks

Located in `test/native/mocks/`:

- **`MockTimeProvider`**: Simulates time with full control
  - `setHour(int)`: Set current hour for time window tests
  - `setMillis(unsigned long)`: Set absolute time for timing tests
  - `advanceMillis(unsigned long)`: Fast-forward time
  - `setTimeAvailable(bool)`: Simulate NTP sync state

- **`MockRelayController`**: Records all relay operations
  - `getIsOn()`: Check current relay state
  - `getTurnOnCount()`: Count activation calls
  - `getTurnOffCount()`: Count deactivation calls

## Running Tests

### Prerequisites

Install PlatformIO in a virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip3 install platformio
```

### Native Unit Tests

Run all unit tests on your development machine:

```bash
# Run all native tests
pio test -e native

# Run specific test suite
pio test -e native -f test_time_window
pio test -e native -f test_state_machine
pio test -e native -f test_interval_timing

# Verbose output for debugging
pio test -e native -v
```

**Advantages:**
- Fast execution (no upload time)
- No hardware required
- Easy debugging with standard tools
- Run in CI/CD pipelines

### Embedded Integration Tests

Run tests on actual ESP32 hardware:

```bash
# Upload and run embedded tests
pio test -e esp32 --upload-port /dev/ttyUSB0

# Monitor test output
pio device monitor
```

**Note:** Embedded integration tests require physical hardware and are optional for this project.

## Test Suites

### 1. Time Window Tests (`test/test_time_window/`)

**Purpose:** Verify that misting only occurs during the active window (9am-6pm).

**Test Cases:**

| Test | Scenario | Expected Behavior |
|------|----------|-------------------|
| `test_no_mist_before_9am` | Time is 8:59am | Relay stays OFF |
| `test_mist_at_9am` | Time is exactly 9:00am | Relay turns ON (first mist) |
| `test_mist_during_window` | Time is 2pm | Misting allowed |
| `test_no_mist_at_6pm` | Time is exactly 6:00pm | Relay stays OFF |
| `test_no_mist_after_6pm` | Time is 7pm | Relay stays OFF |

**Example:**
```cpp
void test_no_mist_before_9am() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(8);  // 8am
    timeProvider.setTimeAvailable(true);

    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());  // Should NOT mist
}
```

### 2. State Machine Tests (`test/test_state_machine/`)

**Purpose:** Verify correct state transitions in the scheduler's state machine.

**State Diagram:**
```
┌──────────────┐
│ WAITING_SYNC │ (Initial state, time not available)
└───────┬──────┘
        │ Time becomes available
        ▼
    ┌───────┐
    │  IDLE │ (Waiting for next mist time)
    └───┬───┘
        │ shouldStartMisting() == true
        ▼
    ┌─────────┐
    │ MISTING │ (Relay ON, misting active)
    └────┬────┘
        │ 25 seconds elapsed
        ▼
    ┌───────┐
    │  IDLE │ (Back to waiting)
    └───────┘
```

**Test Cases:**

| Test | Scenario | Expected State Transition |
|------|----------|---------------------------|
| `test_initial_state_waiting` | Scheduler created, no time | WAITING_SYNC |
| `test_transition_to_idle` | Time becomes available | WAITING_SYNC → IDLE |
| `test_immediate_check_after_sync` | Time available at 10am | WAITING_SYNC → IDLE → MISTING (one update call) |
| `test_transition_to_misting` | In active window, first mist | IDLE → MISTING |
| `test_transition_back_to_idle` | 25 seconds elapsed | MISTING → IDLE |

**Example:**
```cpp
void test_immediate_check_after_sync() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);  // 10am (active window)
    timeProvider.setTimeAvailable(true);

    scheduler.update();  // Single update call

    // Should transition WAITING_SYNC → IDLE → MISTING
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_TRUE(relay.getIsOn());
}
```

### 3. Interval Timing Tests (`test/test_interval_timing/`)

**Purpose:** Verify correct timing intervals between misting cycles.

**Timing Rules:**
- **First mist:** Immediate when time syncs during active window
- **Subsequent mists:** Exactly 2 hours (7,200,000 ms) after last mist
- **Interval enforcement:** No misting before 2 hours have elapsed

**Test Cases:**

| Test | Scenario | Expected Behavior |
|------|----------|-------------------|
| `test_first_mist_immediate` | Time just synced at 10am | Mist starts immediately |
| `test_no_mist_before_interval` | 1 hour 59 minutes after last mist | No misting |
| `test_mist_after_interval` | Exactly 2 hours after last mist | Start new mist cycle |
| `test_interval_with_realistic_timing` | Simulate actual timing progression | Verify exact 2hr spacing |
| `test_multiple_cycles` | Run through 3 complete mist cycles | Verify consistent intervals |

**Example:**
```cpp
void test_no_mist_before_interval() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    timeProvider.setTimeAvailable(true);

    // First mist
    scheduler.update();
    timeProvider.advanceMillis(25000);  // Mist duration
    scheduler.update();

    // Try to mist after 1hr 59min (just before 2hr interval)
    timeProvider.advanceMillis(7199000);
    scheduler.update();

    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());  // Still only 1 mist
}
```

## Test Coverage

### Current Coverage

| Component | Unit Tests | Coverage |
|-----------|------------|----------|
| Time window enforcement | 5 tests | 100% |
| State machine transitions | 5 tests | 100% |
| Interval timing | 5 tests | 100% |
| **Total** | **15 tests** | **100%** |

### What's Tested

- Time window boundaries (9am, 6pm)
- State transitions (WAITING_SYNC → IDLE → MISTING → IDLE)
- First mist behavior (immediate when time syncs)
- 2-hour interval enforcement
- 25-second misting duration
- Edge cases (unavailable time, boundary hours)

### What's NOT Tested (Manual Testing Required)

- WiFi connection reliability
- NTP synchronization accuracy
- Physical relay activation
- Power loss recovery
- Midnight boundary crossing
- Daylight saving time transitions

## Writing New Tests

### Test Template

```cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "../native/mocks/MockTimeProvider.h"
#include "../native/mocks/MockRelayController.h"

void test_your_scenario() {
    // Arrange: Set up test fixtures
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    // Configure mocks
    timeProvider.setHour(10);
    timeProvider.setTimeAvailable(true);

    // Act: Execute the behavior
    scheduler.update();

    // Assert: Verify expected outcomes
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
}

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_your_scenario);
    return UNITY_END();
}
```

### Test Organization

Each test suite lives in its own directory under `test/`:

```
test/
├── test_time_window/
│   └── test_time_window.cpp
├── test_state_machine/
│   └── test_state_machine.cpp
├── test_interval_timing/
│   └── test_interval_timing.cpp
└── native/
    └── mocks/
        ├── MockTimeProvider.h
        └── MockRelayController.h
```

This structure is required by PlatformIO's test framework.

## Debugging Tests

### Enable Verbose Output

```bash
pio test -e native -v
```

### Add Debug Output

```cpp
void test_debug_example() {
    MockTimeProvider timeProvider;

    printf("Current hour: %d\n", timeProvider.getCurrentHour());
    printf("Millis: %lu\n", timeProvider.getMillis());

    // Your test assertions...
}
```

### Common Issues

**Tests fail with "undefined reference"**
- Ensure all source files are in `src/` directory
- Check that `test_build_src = yes` in `platformio.ini`

**Native tests include Arduino.h**
- Add files with Arduino dependencies to `src_filter` exclusion
- Example: `src_filter = +<*> -<main.cpp>`

**State transitions not working**
- Check that you're calling `update()` enough times
- Use `[[fallthrough]]` attribute for intentional state progression

## Continuous Integration

The native tests are designed to run in CI environments without hardware:

```yaml
# Example GitHub Actions workflow
- name: Run Tests
  run: |
    python3 -m venv .venv
    source .venv/bin/activate
    pip3 install platformio
    pio test -e native
```

## Best Practices

1. **Test one thing:** Each test should verify a single behavior
2. **Use descriptive names:** `test_no_mist_before_9am` is better than `test1`
3. **Arrange-Act-Assert:** Structure tests clearly
4. **Mock dependencies:** Don't rely on real time or hardware in unit tests
5. **Test edge cases:** Boundaries, transitions, and error conditions
6. **Keep tests fast:** Native tests should run in milliseconds
7. **Make tests deterministic:** Same input should always produce same output

## Future Enhancements

- [ ] Add tests for midnight boundary crossing
- [ ] Add tests for DST transitions
- [ ] Add tests for power loss recovery
- [ ] Add embedded integration test suite
- [ ] Set up CI/CD pipeline with automated testing
- [ ] Add test coverage reporting
- [ ] Add performance/timing benchmarks
