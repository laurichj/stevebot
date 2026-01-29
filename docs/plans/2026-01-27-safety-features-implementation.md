# Safety Features Implementation Plan

**Date:** 2026-01-27
**Design Document:** [2026-01-27-safety-features-design.md](./2026-01-27-safety-features-design.md)
**Estimated Effort:** 4-6 hours
**Priority:** High (Safety Critical)

## Overview

This implementation plan covers the development of three interconnected safety features:

1. Watchdog Timer
2. Serial Command Interface
3. Non-Volatile Storage (NVS)

The implementation is structured to maintain testability and minimize risk.

## Implementation Phases

### Phase 1: State Storage Abstraction (Foundation)

**Goal:** Create testable storage abstraction layer

**Tasks:**

1. Create `src/IStateStorage.h` interface
   - Define methods: `getLastMistTime()`, `getHasEverMisted()`, `getEnabled()`, `save()`
   - Add documentation comments

2. Create `src/NVSStateStorage.h` and `.cpp`
   - Implement `IStateStorage` using ESP32 `Preferences` library
   - Handle NVS initialization and error cases
   - Add logging for NVS operations

3. Create `test/native/mocks/MockStateStorage.h`
   - Implement `IStateStorage` using in-memory storage (std::map or simple variables)
   - Used by native unit tests

**Acceptance Criteria:**
- [ ] All three files compile without errors
- [ ] `NVSStateStorage` can be instantiated on ESP32
- [ ] `MockStateStorage` can be instantiated in native tests

**Estimated Time:** 1 hour

---

### Phase 2: NVS Integration into MistingScheduler

**Goal:** Add state persistence to scheduler

**Tasks:**

1. Modify `src/MistingScheduler.h`
   - Add `IStateStorage* stateStorage` member
   - Add constructor parameter for `stateStorage` (default `nullptr`)
   - Add `bool schedulerEnabled` member
   - Declare methods: `loadState()`, `saveState()`, `setEnabled()`, `isEnabled()`

2. Modify `src/MistingScheduler.cpp`
   - Implement `loadState()`: read from storage, log results
   - Implement `saveState()`: write to storage
   - Implement `setEnabled()`: update flag, call `saveState()`
   - Implement `isEnabled()`: return `schedulerEnabled`
   - Call `saveState()` after state changes in `startMisting()` and `stopMisting()`
   - Check `schedulerEnabled` in `update()` before automatic misting

3. Update `src/main.cpp`
   - Create `NVSStateStorage` instance
   - Pass to `MistingScheduler` constructor
   - Call `scheduler.loadState()` after WiFi/NTP sync in `setup()`

**Acceptance Criteria:**
- [ ] Scheduler compiles with new storage integration
- [ ] Can create scheduler with `nullptr` storage (backward compatible)
- [ ] Can create scheduler with `NVSStateStorage`
- [ ] State is loaded on startup
- [ ] State is saved after misting events

**Estimated Time:** 1.5 hours

---

### Phase 3: Unit Tests for State Persistence

**Goal:** Verify NVS integration with native tests

**Tasks:**

1. Create `test/test_safety_features/test_state_persistence.cpp`
   - Test: State saved after `startMisting()`
   - Test: State saved after `stopMisting()`
   - Test: State saved after `setEnabled()`
   - Use `MockStateStorage` to verify save calls

2. Create `test/test_safety_features/test_state_recovery.cpp`
   - Test: `loadState()` restores `lastMistTime`
   - Test: `loadState()` restores `hasEverMisted`
   - Test: `loadState()` restores `schedulerEnabled`
   - Test: Default values used if storage returns zeros/false

3. Run native tests
   - `pio test -e native`
   - Verify all new tests pass

**Acceptance Criteria:**
- [ ] All state persistence tests pass
- [ ] All state recovery tests pass
- [ ] Test coverage includes happy path and error cases

**Estimated Time:** 1 hour

---

### Phase 4: Serial Command Interface

**Goal:** Add manual control commands

**Tasks:**

1. Modify `src/MistingScheduler.h`
   - Declare `forceMist()` method
   - Declare `printStatus()` method

2. Modify `src/MistingScheduler.cpp`
   - Implement `forceMist()`:
     - Check if already misting (log error and return if true)
     - Check if enabled (log error and return if false)
     - Call `startMisting()` immediately
   - Implement `printStatus()`:
     - Print current state (IDLE, WAITING_SYNC, MISTING)
     - Print `schedulerEnabled` flag
     - Print `lastMistTime` (formatted as time if available)
     - Print `hasEverMisted`
     - Print next mist estimate if in IDLE state

3. Add `processSerialCommands()` function in `src/main.cpp`
   - Read commands from Serial (non-blocking)
   - Parse: ENABLE, DISABLE, FORCE_MIST, STATUS
   - Call appropriate scheduler methods
   - Print acknowledgment/error messages

4. Call `processSerialCommands()` in `loop()`

**Acceptance Criteria:**
- [ ] `ENABLE` command enables scheduler and saves state
- [ ] `DISABLE` command disables scheduler and saves state
- [ ] `FORCE_MIST` command triggers immediate mist (if enabled and not already misting)
- [ ] `STATUS` command prints detailed scheduler state
- [ ] Unknown commands print error message
- [ ] Commands work when tested via serial monitor

**Estimated Time:** 1 hour

---

### Phase 5: Unit Tests for Commands

**Goal:** Verify command interface behavior

**Tasks:**

1. Create `test/test_safety_features/test_scheduler_enable_disable.cpp`
   - Test: `setEnabled(false)` disables automatic misting
   - Test: `setEnabled(true)` enables automatic misting
   - Test: Disabled scheduler prevents scheduled mists
   - Test: Enabled flag is saved to storage

2. Create `test/test_safety_features/test_force_mist.cpp`
   - Test: `forceMist()` triggers immediate misting
   - Test: `forceMist()` during mist returns error (no action)
   - Test: `forceMist()` when disabled returns error (no action)
   - Test: `forceMist()` updates `lastMistTime`

3. Run native tests
   - `pio test -e native`
   - Verify all new tests pass

**Acceptance Criteria:**
- [ ] All enable/disable tests pass
- [ ] All force mist tests pass
- [ ] Edge cases are covered (force during mist, force when disabled)

**Estimated Time:** 1 hour

---

### Phase 6: Watchdog Timer Implementation

**Goal:** Add system hang protection

**Tasks:**

1. Modify `src/main.cpp` - `setup()` function
   - Include `<esp_task_wdt.h>`
   - Initialize watchdog: `esp_task_wdt_init(10, true)`
   - Add current task to watchdog: `esp_task_wdt_add(NULL)`
   - Check reset reason: `esp_reset_reason()`
   - Log warning if reset was due to watchdog (`ESP_RST_TASK_WDT`)

2. Modify `src/main.cpp` - `loop()` function
   - Add `esp_task_wdt_reset()` as first line
   - Feed watchdog every iteration

**Acceptance Criteria:**
- [ ] Watchdog initializes on startup without errors
- [ ] Watchdog is fed every loop iteration
- [ ] Watchdog reset reason is logged if system was reset by watchdog
- [ ] Code compiles and uploads to ESP32

**Estimated Time:** 30 minutes

---

### Phase 7: Integration Testing on Hardware

**Goal:** Verify all safety features work on real hardware

**Tasks:**

1. Upload firmware to ESP32
   - `pio run -e esp32 -t upload`
   - Monitor serial output

2. Test NVS persistence
   - Wait for first mist cycle
   - Unplug ESP32 (power cycle)
   - Verify system restarts and doesn't immediately re-mist
   - Check serial log for "Loaded state from NVS"

3. Test serial commands
   - Open serial monitor (115200 baud)
   - Send `STATUS` - verify detailed output
   - Send `DISABLE` - verify scheduler stops
   - Send `ENABLE` - verify scheduler resumes
   - Send `FORCE_MIST` - verify immediate mist cycle
   - Send `FORCE_MIST` during mist - verify error message

4. Test watchdog timer
   - Temporarily add infinite loop in `loop()` (e.g., `while(1);`)
   - Upload and monitor
   - Verify system resets after ~10 seconds
   - Verify "watchdog timeout" message on restart
   - Remove infinite loop and re-upload

5. Extended testing
   - Run for 24 hours
   - Monitor for unexpected resets
   - Verify misting happens every 2 hours as expected
   - Verify no memory leaks or timing drift

**Acceptance Criteria:**
- [ ] State persists across power cycles
- [ ] All serial commands work as expected
- [ ] Watchdog triggers and recovers from hangs
- [ ] System runs stably for 24 hours
- [ ] No unexpected resets or failures

**Estimated Time:** 1 hour (+ 24-hour monitoring)

---

### Phase 8: Documentation and Cleanup

**Goal:** Update documentation and mark tasks complete

**Tasks:**

1. Update `README.md`
   - Move completed items from TODO section
   - Document serial commands in "Usage" section
   - Add "Safety Features" section describing watchdog, NVS, and commands

2. Update `test/README.md`
   - Document new safety feature tests
   - Explain `MockStateStorage` usage

3. Review code for:
   - Consistent logging messages
   - Proper error handling
   - Code comments where needed

4. Create summary of changes
   - List all new files
   - List all modified files
   - Summarize behavior changes

**Acceptance Criteria:**
- [ ] README accurately reflects new functionality
- [ ] Test documentation is updated
- [ ] Code is clean and well-commented
- [ ] All files are committed to git

**Estimated Time:** 30 minutes

---

## Testing Strategy Summary

### Native Unit Tests (Phase 3, 5)

Run with: `pio test -e native`

Tests to implement:
- `test_state_persistence.cpp` (3-4 tests)
- `test_state_recovery.cpp` (4 tests)
- `test_scheduler_enable_disable.cpp` (4 tests)
- `test_force_mist.cpp` (4 tests)

**Total:** ~15-16 new unit tests

### Hardware Integration Tests (Phase 7)

Manual testing on ESP32:
- NVS persistence across power cycles
- Serial command interface
- Watchdog reset recovery
- 24-hour stability test

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| NVS corruption on production device | Graceful defaults, log warnings, allow manual override |
| Watchdog triggers too frequently | 10-second timeout is conservative; WiFi/NTP operations should complete within this |
| State not saved before power loss | NVS writes are fast (~1ms); save after every state change |
| Serial commands interfere with scheduler | Non-blocking command processing; commands only processed when data available |
| Breaking existing functionality | Backward compatible (storage parameter defaults to nullptr); extensive unit tests |

---

## Dependencies

### External Libraries
- `Preferences.h` - ESP32 Arduino framework (already available)
- `esp_task_wdt.h` - ESP32 Arduino framework (already available)

### Internal Dependencies
- Requires existing `ITimeProvider` and `IRelayController` interfaces
- Requires existing `MistingScheduler` class
- Requires existing test infrastructure (PlatformIO native tests)

---

## Success Criteria

This implementation is complete when:

1. ✅ All 15+ native unit tests pass
2. ✅ State persists across power cycles (verified on hardware)
3. ✅ All four serial commands work correctly
4. ✅ Watchdog can trigger and recover from hangs
5. ✅ System runs for 24 hours without unexpected failures
6. ✅ Documentation is updated and accurate
7. ✅ Code is committed with proper commit messages

---

## Rollback Plan

If issues are discovered during implementation:

1. Each phase is isolated and can be rolled back independently
2. Storage abstraction (Phase 1-2) can be disabled by passing `nullptr` to scheduler
3. Commands (Phase 4) can be removed from `loop()` without affecting scheduler
4. Watchdog (Phase 6) can be disabled by removing initialization code

Backward compatibility is maintained throughout.

---

## Post-Implementation

After successful deployment, monitor for:
- Unexpected watchdog resets (indicates bugs in main loop)
- NVS write failures (indicates storage issues)
- User feedback on command interface usability

Future enhancements (documented in design):
- Web interface for remote control
- MQTT integration
- Historical misting log in NVS

---

## Implementation Checklist

Use this checklist during implementation:

### Phase 1: State Storage Abstraction
- [ ] Create `IStateStorage.h` interface
- [ ] Create `NVSStateStorage.h` and `.cpp`
- [ ] Create `MockStateStorage.h`
- [ ] Verify compilation

### Phase 2: NVS Integration
- [ ] Modify `MistingScheduler.h`
- [ ] Modify `MistingScheduler.cpp`
- [ ] Update `main.cpp`
- [ ] Verify compilation and upload

### Phase 3: State Persistence Tests
- [ ] Create `test_state_persistence.cpp`
- [ ] Create `test_state_recovery.cpp`
- [ ] Run `pio test -e native`
- [ ] All tests pass

### Phase 4: Serial Commands
- [ ] Add `forceMist()` to scheduler
- [ ] Add `printStatus()` to scheduler
- [ ] Add `processSerialCommands()` to main
- [ ] Integrate into `loop()`

### Phase 5: Command Tests
- [ ] Create `test_scheduler_enable_disable.cpp`
- [ ] Create `test_force_mist.cpp`
- [ ] Run `pio test -e native`
- [ ] All tests pass

### Phase 6: Watchdog Timer
- [ ] Add watchdog init to `setup()`
- [ ] Add watchdog feed to `loop()`
- [ ] Add reset reason logging
- [ ] Verify compilation and upload

### Phase 7: Hardware Testing
- [ ] Test NVS persistence
- [ ] Test serial commands
- [ ] Test watchdog reset
- [ ] 24-hour stability test

### Phase 8: Documentation
- [ ] Update `README.md`
- [ ] Update `test/README.md`
- [ ] Review and clean code
- [ ] Commit changes

---

## Estimated Timeline

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| 1. State Storage Abstraction | 1 hour | None |
| 2. NVS Integration | 1.5 hours | Phase 1 |
| 3. State Persistence Tests | 1 hour | Phase 2 |
| 4. Serial Commands | 1 hour | Phase 2 |
| 5. Command Tests | 1 hour | Phase 4 |
| 6. Watchdog Timer | 0.5 hours | None (can be parallel) |
| 7. Hardware Testing | 1 hour + 24hr | Phases 2, 4, 6 |
| 8. Documentation | 0.5 hours | Phase 7 |

**Total Active Development:** 6-7 hours
**Total with Testing:** 30-31 hours (includes 24-hour monitoring)

---

## Notes

- Phases 1-5 can be developed and tested entirely with native unit tests (no hardware required)
- Phase 6 (watchdog) is independent and can be implemented in parallel
- Phase 7 requires physical ESP32 hardware
- Implementation can be paused after any phase if needed
- Each phase produces working, testable code
