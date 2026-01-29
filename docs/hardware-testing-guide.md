# Hardware Testing Guide

**Date:** 2026-01-27
**Purpose:** Step-by-step procedures for validating safety features on ESP32 hardware
**Prerequisites:** Firmware uploaded to ESP32, serial monitor available at 115200 baud

## Overview

This guide covers manual testing procedures for the three safety features implemented in Stevebot:

1. **Non-Volatile Storage (NVS)** - State persistence across power cycles
2. **Serial Command Interface** - Manual control commands
3. **Watchdog Timer** - System hang protection

All tests require physical access to the ESP32 and the ability to monitor serial output.

---

## Test Setup

### Required Equipment

- ESP32 with uploaded firmware
- USB cable for power and serial communication
- Serial monitor (Arduino IDE, PlatformIO, or `screen`)
- Power switch or ability to safely disconnect/reconnect power

### Initial Setup

1. **Upload latest firmware:**
   ```bash
   pio run -e esp32 --target upload
   ```

2. **Open serial monitor:**
   ```bash
   pio device monitor
   ```
   Or use Arduino IDE Serial Monitor at 115200 baud.

3. **Wait for startup:** System should display:
   - WiFi connection status
   - NTP synchronization messages
   - Initial scheduler state

---

## Test 1: NVS Persistence (Power Cycle Test)

**Purpose:** Verify that scheduler state persists across power cycles and prevents duplicate misting.

### Procedure

1. **Wait for first mist cycle:**
   - Monitor serial output until you see:
     ```
     [HH:MM:SS] Starting mist cycle...
     [HH:MM:SS] Mist cycle complete
     ```
   - Note the timestamp of the mist cycle.

2. **Verify state is saved:**
   - Look for log message:
     ```
     [HH:MM:SS] State saved to NVS
     ```

3. **Power cycle the ESP32:**
   - Unplug the USB cable or power supply
   - Wait 5 seconds
   - Reconnect power

4. **Check startup logs:**
   - Look for:
     ```
     [HH:MM:SS] Loaded state from NVS
     [HH:MM:SS]   Last mist time: <timestamp>
     [HH:MM:SS]   Has ever misted: true
     [HH:MM:SS]   Scheduler enabled: true
     ```

5. **Send STATUS command:**
   ```
   STATUS
   ```
   - Verify output shows `lastMistTime` matching the pre-power-cycle value
   - Verify `schedulerEnabled: true`
   - Verify `hasEverMisted: true`

### Success Criteria

- [ ] State is saved after mist cycle completes
- [ ] State is restored on startup after power cycle
- [ ] `lastMistTime` matches pre-power-cycle value
- [ ] System does NOT immediately re-mist after restart
- [ ] Next mist estimate is approximately 2 hours from last mist

### Failure Modes

| Symptom | Possible Cause | Action |
|---------|---------------|---------|
| "Failed to load state from NVS" | First boot or NVS corruption | Expected on first boot; if recurring, check NVS initialization |
| Immediate re-misting after restart | State not restored correctly | Check `lastMistTime` value in STATUS output |
| "Using default state values" | NVS read failure | Check for NVS errors in logs; may indicate storage issues |

---

## Test 2: Serial Commands

**Purpose:** Verify all manual control commands work correctly.

### Test 2A: STATUS Command

1. **Send command:**
   ```
   STATUS
   ```

2. **Verify output includes:**
   - Current state (WAITING_SYNC, IDLE, or MISTING)
   - Scheduler enabled/disabled status
   - Last mist time (formatted timestamp if available)
   - Has ever misted flag
   - Next scheduled mist time (if in IDLE state)

**Expected output example:**
```
===== MISTING SCHEDULER STATUS =====
State: IDLE
Scheduler enabled: true
Last mist time: 2026-01-27 14:30:00
Has ever misted: true
Next mist estimate: 2026-01-27 16:30:00
====================================
```

### Test 2B: DISABLE Command

1. **Send command:**
   ```
   DISABLE
   ```

2. **Verify acknowledgment:**
   ```
   OK: Scheduler disabled
   ```

3. **Check state persistence:**
   - Send `STATUS` command
   - Verify `schedulerEnabled: false`

4. **Wait through a scheduled mist time:**
   - System should NOT activate misting
   - No "Starting mist cycle" messages

5. **Verify behavior during current mist:**
   - If misting is active, send `DISABLE`
   - Current mist cycle should complete normally
   - Future mists should be prevented

**Success Criteria:**
- [ ] Acknowledgment message received
- [ ] STATUS shows scheduler disabled
- [ ] No automatic misting occurs after disable
- [ ] State persists across STATUS commands

### Test 2C: ENABLE Command

1. **Send command:**
   ```
   ENABLE
   ```

2. **Verify acknowledgment:**
   ```
   OK: Scheduler enabled
   ```

3. **Check state persistence:**
   - Send `STATUS` command
   - Verify `schedulerEnabled: true`

4. **Wait for next scheduled time:**
   - System should resume automatic misting
   - Respects 2-hour interval from last mist

**Success Criteria:**
- [ ] Acknowledgment message received
- [ ] STATUS shows scheduler enabled
- [ ] Automatic misting resumes at next scheduled time
- [ ] State persists across STATUS commands

### Test 2D: FORCE_MIST Command

1. **Ensure scheduler is enabled:**
   ```
   STATUS
   ```
   - If disabled, send `ENABLE` first

2. **Send command:**
   ```
   FORCE_MIST
   ```

3. **Verify immediate misting:**
   - Acknowledgment: `OK: Forcing mist cycle`
   - Log message: `[HH:MM:SS] Manual mist triggered`
   - Relay activates immediately
   - Mist runs for 25 seconds

4. **Verify lastMistTime update:**
   - After mist completes, send `STATUS`
   - `lastMistTime` should match forced mist timestamp

**Test edge cases:**

5. **FORCE_MIST during active mist:**
   - While misting is active, send `FORCE_MIST` again
   - Expected: `ERROR: Cannot force mist while already misting`
   - Relay stays active, mist continues normally

6. **FORCE_MIST when disabled:**
   - Send `DISABLE`
   - Send `FORCE_MIST`
   - Expected: `ERROR: Scheduler is disabled`
   - Relay does NOT activate

**Success Criteria:**
- [ ] Immediate misting triggered when enabled and idle
- [ ] Error message when already misting
- [ ] Error message when scheduler disabled
- [ ] lastMistTime updated after forced mist
- [ ] Subsequent scheduled mists delayed by 2 hours from forced mist

### Test 2E: Unknown Commands

1. **Send invalid command:**
   ```
   INVALID_COMMAND
   ```

2. **Verify error handling:**
   ```
   ERROR: Unknown command
   ```

3. **Test variations:**
   - Lowercase: `enable` → Should work (case-insensitive)
   - Typo: `ENABBLE` → Should return error
   - Empty line → Should be ignored

**Success Criteria:**
- [ ] Unknown commands return error message
- [ ] Commands are case-insensitive
- [ ] Invalid input doesn't crash system

---

## Test 3: Watchdog Timer (Deliberate Hang Test)

**Purpose:** Verify watchdog timer triggers and recovers from system hangs.

**WARNING:** This test will cause a system reset. Ensure relay is not controlling critical equipment.

### Procedure

1. **Prepare test firmware:**
   - Temporarily modify `src/main.cpp` loop():
   ```cpp
   void loop() {
       esp_task_wdt_reset();  // Keep this line

       // Add deliberate hang after 30 seconds
       static unsigned long hangTime = millis() + 30000;
       if (millis() > hangTime) {
           Serial.println("DELIBERATE HANG: Testing watchdog...");
           while(1) {
               // Infinite loop - watchdog should trigger
               delay(100);
           }
       }

       // ... rest of loop code ...
   }
   ```

2. **Upload modified firmware:**
   ```bash
   pio run -e esp32 --target upload
   ```

3. **Monitor serial output:**
   - Wait 30 seconds for hang message:
     ```
     DELIBERATE HANG: Testing watchdog...
     ```
   - Serial output stops (system is hung)

4. **Wait for watchdog reset:**
   - Watchdog timeout: 10 seconds
   - System should automatically reset

5. **Verify watchdog reset detection:**
   - After restart, look for:
     ```
     [HH:MM:SS] WARNING: System restarted due to watchdog timeout
     ```

6. **Check system recovery:**
   - WiFi reconnects
   - NTP resyncs
   - Scheduler resumes normal operation
   - Relay defaults to OFF
   - State restored from NVS

7. **Restore normal firmware:**
   - Remove the deliberate hang code
   - Re-upload clean firmware

### Success Criteria

- [ ] System hangs after 30 seconds (as programmed)
- [ ] Watchdog triggers within ~10 seconds of hang
- [ ] System resets automatically
- [ ] Reset reason detected and logged
- [ ] Relay is OFF after reset
- [ ] State restored from NVS
- [ ] Scheduler resumes operation normally

### Failure Modes

| Symptom | Possible Cause | Action |
|---------|---------------|---------|
| System never resets | Watchdog not initialized | Check `esp_task_wdt_init()` in setup() |
| Reset but no warning message | Reset reason check missing | Verify `esp_reset_reason()` logic |
| Continuous reset loop | Watchdog feed missing in loop | Add `esp_task_wdt_reset()` at start of loop() |
| State lost after reset | NVS not saving | Run Test 1 first to verify NVS |

---

## Test 4: 24-Hour Stability Test

**Purpose:** Verify system runs reliably over extended period without unexpected failures.

### Procedure

1. **Start monitoring session:**
   - Upload firmware
   - Begin logging serial output to file:
     ```bash
     pio device monitor | tee stability-test.log
     ```
   - Record start time and date

2. **Monitor for 24 hours:**
   - System should mist every 2 hours during active window (9am-6pm)
   - Expected mist cycles: ~5 per day (9am, 11am, 1pm, 3pm, 5pm)

3. **Check for anomalies:**
   - Unexpected resets (watchdog triggers)
   - NVS errors
   - Time synchronization failures
   - Missed mist cycles
   - Duplicate misting (interval violations)

4. **Analyze logs:**
   ```bash
   # Count mist cycles
   grep "Starting mist cycle" stability-test.log | wc -l

   # Check for watchdog resets
   grep "watchdog timeout" stability-test.log

   # Check for NVS errors
   grep "Failed to load state" stability-test.log

   # Check for time sync issues
   grep "Time sync failed" stability-test.log
   ```

5. **Verify timing accuracy:**
   - Extract mist timestamps from log
   - Calculate intervals between mists
   - Should be approximately 2 hours (7200 seconds ± 60 seconds)

### Success Criteria

- [ ] System runs for 24 hours without crashes
- [ ] No unexpected watchdog resets
- [ ] 4-5 mist cycles per day (during 9am-6pm window)
- [ ] All intervals approximately 2 hours
- [ ] No NVS errors
- [ ] No memory leaks (check for decreasing free memory)
- [ ] Time remains synchronized (no drift >5 seconds)

### Failure Modes

| Symptom | Possible Cause | Action |
|---------|---------------|---------|
| Unexpected resets | Loop hang or WiFi timeout | Review code around reset; check WiFi stability |
| Missed mist cycles | Scheduler logic error | Review state machine transitions |
| Duplicate misting | Interval calculation error | Check 2-hour interval logic |
| Time drift | NTP sync failing | Check network connectivity; verify NTP server reachability |
| NVS errors | Flash wear or corruption | Consider reducing save frequency if excessive |

---

## Test 5: Integration Test (All Features Together)

**Purpose:** Verify all safety features work together correctly.

### Scenario: Simulated Power Failure During Mist

1. **Trigger a mist cycle:**
   ```
   FORCE_MIST
   ```

2. **During misting (within 25 seconds):**
   - Power cycle the ESP32 mid-mist

3. **After restart:**
   - Verify relay is OFF (safety default)
   - Verify state restored from NVS
   - Verify `lastMistTime` reflects the interrupted mist
   - Next mist should be 2 hours from interrupted mist

### Scenario: Watchdog Protection While Disabled

1. **Disable scheduler:**
   ```
   DISABLE
   ```

2. **Trigger watchdog reset:**
   - Upload hang test firmware (from Test 3)
   - Let watchdog reset system

3. **After restart:**
   - Send `STATUS`
   - Verify scheduler still disabled (state persisted)
   - System does not mist automatically

### Scenario: Manual Override After Power Cycle

1. **Trigger mist and power cycle:**
   - Send `FORCE_MIST`
   - Wait for completion
   - Power cycle

2. **After restart:**
   - Immediately send `FORCE_MIST` again
   - Should work (manual override ignores automatic schedule)

3. **Verify state:**
   - Send `STATUS`
   - `lastMistTime` should reflect most recent forced mist
   - Next automatic mist: 2 hours from last mist

### Success Criteria

- [ ] All features work correctly in combination
- [ ] State persistence survives unexpected resets
- [ ] Manual commands work regardless of scheduler state
- [ ] Watchdog protection doesn't interfere with normal operation
- [ ] No edge cases cause undefined behavior

---

## Troubleshooting Guide

### Common Issues

#### Serial Commands Not Working

**Symptoms:**
- Commands ignored
- No acknowledgment messages

**Checks:**
1. Verify baud rate: 115200
2. Ensure newline is sent (Enter key)
3. Check for serial buffer issues (send one command at a time)
4. Monitor for `processSerialCommands()` being called in loop

#### State Not Persisting

**Symptoms:**
- "Failed to load state from NVS"
- State resets to defaults after power cycle

**Checks:**
1. First boot is expected to show defaults
2. Check for NVS initialization errors
3. Verify `saveState()` called after state changes
4. Check available flash memory (NVS partition)

#### Watchdog False Triggers

**Symptoms:**
- Frequent unexpected resets
- "System restarted due to watchdog timeout" messages

**Checks:**
1. Verify `esp_task_wdt_reset()` at start of loop()
2. Check for blocking operations in loop (WiFi delays, Serial reads)
3. Increase watchdog timeout if necessary (currently 10 seconds)
4. Review any new code that might block loop execution

#### Relay Not Activating

**Symptoms:**
- Mist cycle logs appear but relay doesn't activate

**Checks:**
1. Verify relay wiring (pin 13)
2. Check relay power supply
3. Test relay directly with manual GPIO toggle
4. Verify `GPIORelayController` initialization

---

## Test Results Template

Use this template to document test results:

```
Hardware Testing Results
Date: YYYY-MM-DD
Firmware Version: <git commit hash>
Hardware: <ESP32 board model>

Test 1: NVS Persistence
Status: [PASS/FAIL]
Notes:

Test 2A: STATUS Command
Status: [PASS/FAIL]
Notes:

Test 2B: DISABLE Command
Status: [PASS/FAIL]
Notes:

Test 2C: ENABLE Command
Status: [PASS/FAIL]
Notes:

Test 2D: FORCE_MIST Command
Status: [PASS/FAIL]
Notes:

Test 2E: Unknown Commands
Status: [PASS/FAIL]
Notes:

Test 3: Watchdog Timer
Status: [PASS/FAIL]
Notes:

Test 4: 24-Hour Stability
Status: [PASS/FAIL]
Duration: XX hours
Mist Cycles: XX
Unexpected Resets: XX
Notes:

Test 5: Integration Test
Status: [PASS/FAIL]
Notes:

Overall Result: [PASS/FAIL]
Issues Found:
-
-

Recommendations:
-
-
```

---

## Next Steps

After completing all hardware tests:

1. **Document any issues found** in GitHub issues
2. **Update firmware** if bugs are discovered
3. **Run regression tests** after fixes (re-run all hardware tests)
4. **Deploy to production** once all tests pass
5. **Monitor in production** for first 48 hours for unexpected behavior

For production deployment, consider:
- Setting up remote logging (MQTT, syslog)
- Implementing periodic health checks
- Creating automated test procedures
- Documenting recovery procedures for field issues
