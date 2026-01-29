# Safety Features Design

**Date:** 2026-01-27
**Status:** Design Complete
**Priority:** High (Safety Critical)

## Overview

This design document outlines three interconnected safety features for the misting scheduler system:

1. **Watchdog Timer** - Prevents relay from sticking ON if system hangs
2. **Manual Override Commands** - Enables emergency control via serial interface
3. **Non-Volatile Storage** - Preserves state across power cycles

These features work together to create a robust safety system that prevents overwatering, enables manual intervention, and recovers gracefully from power failures.

## System Architecture

### Three-Layer Safety Design

```
┌─────────────────────────────────────────────────┐
│         Watchdog Timer Layer                    │
│  - Monitors main loop execution                 │
│  - 10-second timeout → system reset             │
│  - Fed every loop iteration                     │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│      Command Interface Layer                    │
│  - Serial command parser                        │
│  - Commands: ENABLE, DISABLE, FORCE_MIST, STATUS│
│  - Non-blocking command processing              │
└─────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────┐
│      State Persistence Layer                    │
│  - ESP32 NVS (Non-Volatile Storage)            │
│  - Persisted: lastMistTime, hasEverMisted,     │
│    schedulerEnabled                             │
│  - Saved after each state change                │
└─────────────────────────────────────────────────┘
```

### Integration Points

- **Watchdog:** Integrates into `loop()` function
- **Command Parser:** Integrates into `loop()` function
- **NVS Storage:** Integrates into `MistingScheduler` class via new `IStateStorage` interface
- All three layers operate independently but complement each other

## Component Designs

### 1. Watchdog Timer Implementation

#### ESP32 Hardware Watchdog

Uses ESP32's built-in Task Watchdog Timer (TWDT):

```cpp
// In setup()
esp_task_wdt_init(10, true);  // 10 sec timeout, trigger panic/reset
esp_task_wdt_add(NULL);        // Monitor this task

// Check if this is a watchdog-triggered reset
esp_reset_reason_t reason = esp_reset_reason();
if (reason == ESP_RST_TASK_WDT) {
    logWithTimestamp("WARNING: System restarted due to watchdog timeout");
}
```

#### Loop Integration

```cpp
void loop() {
    esp_task_wdt_reset();  // Feed watchdog FIRST

    processSerialCommands();  // Check for manual commands
    scheduler.update();        // Run scheduler logic

    delay(100);
}
```

#### Safety Guarantees

- If main loop hangs for >10 seconds → System resets
- If `scheduler.update()` infinite loops → System resets
- If WiFi connection blocks indefinitely → System resets
- Relay always defaults to OFF after reset (existing behavior in `GPIORelayController`)

### 2. Serial Command Interface

#### Command Structure

Simple text-based commands (easy to type in serial monitor):
- Case-insensitive
- Newline-terminated
- Commands: `ENABLE`, `DISABLE`, `FORCE_MIST`, `STATUS`

#### Implementation

```cpp
void processSerialCommands() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "ENABLE") {
        scheduler.setEnabled(true);
        Serial.println("OK: Scheduler enabled");
    } else if (cmd == "DISABLE") {
        scheduler.setEnabled(false);
        Serial.println("OK: Scheduler disabled");
    } else if (cmd == "FORCE_MIST") {
        scheduler.forceMist();
        Serial.println("OK: Forcing mist cycle");
    } else if (cmd == "STATUS") {
        scheduler.printStatus();
    } else {
        Serial.println("ERROR: Unknown command");
    }
}
```

#### New MistingScheduler Methods

```cpp
class MistingScheduler {
public:
    // New safety/control methods
    void setEnabled(bool enabled);
    bool isEnabled() const { return schedulerEnabled; }
    void forceMist();     // Manually trigger a mist cycle
    void printStatus();   // Print detailed status to Serial

private:
    bool schedulerEnabled;  // New flag, persisted to NVS
};
```

#### Command Behavior

- **ENABLE:** Sets `schedulerEnabled = true`, saves to NVS, allows automatic misting
- **DISABLE:** Sets `schedulerEnabled = false`, saves to NVS, prevents automatic misting (won't stop current mist, just prevents new ones)
- **FORCE_MIST:** Bypasses schedule, immediately starts a 25-second mist cycle (only works if enabled)
- **STATUS:** Prints state, last mist time, enabled/disabled, next mist time estimate

### 3. Non-Volatile Storage (NVS)

#### ESP32 NVS Library

Uses `Preferences.h` (Arduino-friendly wrapper around ESP-IDF NVS):

- **Namespace:** `"misting"` (isolated storage for this app)
- **Stored values:**
  - `lastMistTime` (unsigned long)
  - `hasEverMisted` (bool)
  - `enabled` (bool)

#### Storage Abstraction Interface

To enable testing with native unit tests:

```cpp
// src/IStateStorage.h
class IStateStorage {
public:
    virtual ~IStateStorage() {}
    virtual unsigned long getLastMistTime() = 0;
    virtual bool getHasEverMisted() = 0;
    virtual bool getEnabled() = 0;
    virtual void save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) = 0;
};
```

#### Production Implementation

```cpp
// src/NVSStateStorage.h
class NVSStateStorage : public IStateStorage {
public:
    unsigned long getLastMistTime() override {
        preferences.begin("misting", true);  // true = read-only
        unsigned long val = preferences.getULong("lastMist", 0);
        preferences.end();
        return val;
    }

    // Similar for getHasEverMisted(), getEnabled()

    void save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) override {
        preferences.begin("misting", false);  // false = read-write
        preferences.putULong("lastMist", lastMistTime);
        preferences.putBool("hasMisted", hasEverMisted);
        preferences.putBool("enabled", enabled);
        preferences.end();
    }

private:
    Preferences preferences;
};
```

#### MistingScheduler Integration

```cpp
class MistingScheduler {
public:
    MistingScheduler(ITimeProvider* timeProvider,
                     IRelayController* relayController,
                     IStateStorage* stateStorage = nullptr,  // New parameter
                     LogCallback logger = nullptr);

    void loadState();   // Call in setup(), after WiFi/NTP sync
    void saveState();   // Call after state changes

private:
    IStateStorage* stateStorage;
};
```

#### When to Save

- After `startMisting()`: Save `lastMistTime` and `hasEverMisted`
- After `stopMisting()`: Save state (redundant but safe)
- After `setEnabled()`: Save `schedulerEnabled` flag

NVS writes are fast (~1ms) and NVS has 100,000+ write cycle endurance.

#### Startup Sequence

```cpp
void setup() {
    // ... WiFi, NTP setup ...

    scheduler.loadState();  // Restore state BEFORE starting scheduler

    // Scheduler now knows if it misted recently, preventing double-mist
}
```

## Error Handling & Edge Cases

### Critical Safety Scenarios

#### 1. Watchdog Trigger During Misting

- **Problem:** System hangs while relay is ON
- **Solution:** Watchdog resets system → relay defaults to OFF in `GPIORelayController` constructor
- **Recovery:** Scheduler loads state from NVS, sees recent `lastMistTime`, won't re-mist for 2 hours

#### 2. Power Loss During Misting

- **Problem:** Power cut at any time
- **Solution:** On restart, relay is OFF by default, `lastMistTime` is restored from NVS
- **Edge case:** If power lost before `saveState()` completes, worst case is misting happens slightly earlier than 2-hour interval (safe)

#### 3. NVS Corruption or Initialization Failure

- **Problem:** NVS read fails (rare but possible)
- **Solution:** Default to safe values (`enabled=true`, `lastMistTime=0`, `hasEverMisted=false`)
- **Result:** System will mist immediately when time syncs (same as first boot)

#### 4. WiFi/NTP Failure After Boot

- **Problem:** Time becomes unavailable after initial sync
- **Solution:** Scheduler already handles this (stays in `WAITING_SYNC` state, relay stays OFF)
- **Watchdog:** Still fed, system doesn't reset (WiFi retry logic in setup is time-limited)

#### 5. Manual Commands During Automated Mist

- **DISABLE during mist:** Current mist completes, future mists blocked
- **FORCE_MIST during mist:** Ignored with message "Already misting"
- **ENABLE when disabled:** Future mists allowed, respects 2-hour interval

#### 6. Serial Buffer Overflow

- **Problem:** Many commands sent quickly
- **Solution:** `Serial.readStringUntil()` handles this, processes one command per loop iteration

### Error Logging Strategy

All safety events logged via `logWithTimestamp()`:

- Watchdog resets: `"WARNING: System restarted due to watchdog timeout"`
- NVS errors: `"WARNING: Failed to load state from NVS, using defaults"`
- Command errors: `"ERROR: Unknown command"` or `"ERROR: Cannot force mist while misting"`

## Testing Strategy

### Unit Testing Approach

#### 1. Watchdog Testing

- **Challenge:** Can't test actual watchdog resets in native tests (ESP32-specific)
- **Solution:** Test the integration points and state recovery
  - Test `loadState()` after simulated watchdog reset
  - Verify relay is OFF after scheduler construction
  - Test that `lastMistTime` restoration prevents immediate re-misting

#### 2. Command Interface Testing

- **Native Tests:** Test new `MistingScheduler` methods directly
  - `test_setEnabled_disables_automatic_misting()`
  - `test_forceMist_triggers_immediate_cycle()`
  - `test_forceMist_blocked_when_already_misting()`
  - `test_disabled_scheduler_prevents_automatic_mist()`
- **Manual Tests:** Use actual serial monitor to verify commands work

#### 3. NVS Persistence Testing

- **Challenge:** Native tests can't access ESP32 NVS
- **Solution:** Use `IStateStorage` abstraction
  - `NVSStateStorage` for production (uses `Preferences`)
  - `MockStateStorage` for testing (uses in-memory map)
- **Native Tests:** Test persistence logic with mock
  - `test_state_persisted_after_misting()`
  - `test_state_restored_on_init()`
  - `test_enabled_flag_persists()`

#### 4. Integration Testing on Hardware

- **Power cycle tests:** Verify state recovery after reboot
- **Watchdog test:** Deliberately hang the system (e.g., `while(1);` in loop), verify reset
- **Command tests:** Send all commands via serial monitor, verify behavior
- **24-hour endurance test:** Run continuously, monitor for unexpected resets

### Test Organization

```
test/
├── test_safety_features/
│   ├── test_scheduler_enable_disable.cpp
│   ├── test_force_mist.cpp
│   ├── test_state_persistence.cpp
│   └── test_state_recovery.cpp
└── native/mocks/
    └── MockStateStorage.h (new)
```

## Implementation Considerations

### Code Changes Summary

#### New Files

- `src/IStateStorage.h` - Storage abstraction interface
- `src/NVSStateStorage.h` - Production NVS implementation
- `test/native/mocks/MockStateStorage.h` - Test mock
- `test/test_safety_features/*.cpp` - Safety feature tests (4 test files)

#### Modified Files

- `src/MistingScheduler.h` - Add enable/disable, force mist, state persistence methods
- `src/MistingScheduler.cpp` - Implement new methods, integrate state storage
- `src/main.cpp` - Add watchdog init, command parser, state loading
- `README.md` - Update TODO checklist, add safety features documentation

### Dependencies

- `Preferences.h` - Already available in ESP32 Arduino framework
- `esp_task_wdt.h` - Already available in ESP32 Arduino framework
- **No new external libraries required**

### Migration Path

#### Backward Compatibility

- Existing deployments will work without changes
- On first boot with new firmware: NVS initializes with defaults
- No breaking changes to existing interfaces

#### Deployment Steps

1. Upload new firmware via PlatformIO
2. Monitor serial output for "Loaded state from NVS" (or defaults message)
3. Test commands: `STATUS`, `DISABLE`, `ENABLE`
4. Verify watchdog: Send a hang command for testing (optional)

### Performance Impact

- **NVS writes:** ~1ms per save, only on state changes (not in hot path)
- **Command parsing:** Only when serial data available (non-blocking)
- **Watchdog feed:** Negligible (<1μs per loop iteration)
- **Memory:** ~100 bytes for command buffer, ~50 bytes for NVS storage
- **Overall:** No measurable impact on scheduler performance

## Future Enhancements

Once this is working, possible additions:

- Web interface for remote control (reuse enable/disable/force commands)
- MQTT commands (same command set, different transport)
- Misting history log in NVS (circular buffer of last N mist times)

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| Implement all three features together | They're closely related and form a cohesive safety system |
| Watchdog timeout: 10 seconds | Allows for WiFi delays during startup while still catching hangs quickly |
| Feed watchdog every loop iteration | Simplest and most reliable - proves main loop is running |
| Reset and resume on watchdog trigger | Clean restart is the standard pattern; relay defaults to OFF on startup |
| Command interface: ENABLE, DISABLE, FORCE_MIST, STATUS | Provides useful emergency control without over-complicating |
| Persist lastMistTime, hasEverMisted, schedulerEnabled | Full state recovery while keeping storage lightweight |
| Abstract storage with IStateStorage | Enables native unit testing without ESP32 hardware |

## Conclusion

This three-layer safety design provides comprehensive protection against the main failure modes of an automated misting system:

1. **Hardware failures** (watchdog + relay-off default)
2. **Power failures** (NVS state persistence)
3. **Operator intervention needs** (serial commands)

The design maintains the existing clean architecture, adds minimal overhead, and enables thorough testing through abstraction interfaces.
