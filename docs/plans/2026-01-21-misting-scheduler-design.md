# Misting Scheduler Design

**Date:** 2026-01-21
**Status:** Approved
**Purpose:** Implement automated misting schedule for iguana habitat humidity control

## Overview

Stevebot will automatically control a relay-connected misting system to maintain proper humidity levels in an iguana enclosure. The system activates the mister for 25 seconds every 2 hours during daylight hours (9am to 6pm).

## Requirements

- Mist for 25 seconds duration
- Run every 2 hours during active window
- Active window: 9am to 5:59:59pm (allows 25-second overflow past 6pm)
- First boot: mist immediately when entering window
- Subsequent boots: mist if >2 hours since last mist
- Safe startup: relay OFF until ready to mist
- Monitor and log all misting events with timestamps

## Architecture

### State Machine

Three states manage the misting lifecycle:

```cpp
enum MisterState {
  WAITING_SYNC,   // Time not yet synchronized, can't schedule
  IDLE,           // Waiting for next misting time
  MISTING         // Actively running the mister
};
```

**State Transitions:**
- `WAITING_SYNC` → `IDLE`: When NTP synchronization completes
- `IDLE` → `MISTING`: When conditions met (in window, interval elapsed)
- `MISTING` → `IDLE`: After 25-second duration completes

### State Variables

```cpp
MisterState currentState = WAITING_SYNC;
unsigned long lastMistTime = 0;        // millis() when last mist started
unsigned long mistStartTime = 0;       // millis() when current mist started
const unsigned long MIST_DURATION = 25000;      // 25 seconds
const unsigned long MIST_INTERVAL = 7200000;    // 2 hours
```

### Time Window Logic

```cpp
bool isInActiveWindow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;  // Can't determine time, not safe to mist
  }

  int hour = timeinfo.tm_hour;
  return (hour >= 9 && hour < 18);  // 9am to 5:59:59pm
}

bool shouldStartMisting() {
  if (!isInActiveWindow()) return false;
  if (currentState != IDLE) return false;

  // First mist (or after boot with no stored state)
  if (lastMistTime == 0) return true;

  // Check if 2 hours have passed
  unsigned long elapsed = millis() - lastMistTime;
  return (elapsed >= MIST_INTERVAL);
}
```

### State Machine Implementation

```cpp
void updateMisterStateMachine() {
  switch (currentState) {
    case WAITING_SYNC:
      if (timeInitialized) {
        currentState = IDLE;
        lastMistTime = 0;  // Treat as never misted
      }
      break;

    case IDLE:
      if (shouldStartMisting()) {
        startMisting();
      }
      break;

    case MISTING:
      unsigned long elapsed = millis() - mistStartTime;
      if (elapsed >= MIST_DURATION) {
        stopMisting();
      }
      break;
  }
}

void startMisting() {
  digitalWrite(RELAY_PIN, HIGH);
  mistStartTime = millis();
  lastMistTime = millis();
  currentState = MISTING;
  logWithTimestamp("MIST START");
}

void stopMisting() {
  digitalWrite(RELAY_PIN, LOW);
  currentState = IDLE;
  logWithTimestamp("MIST STOP");

  // Log next scheduled time
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now = mktime(&timeinfo);
    time_t nextMist = now + (MIST_INTERVAL / 1000);
    struct tm* nextTimeinfo = localtime(&nextMist);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Next mist: %04d-%02d-%02d %02d:%02d:%02d",
             nextTimeinfo->tm_year + 1900,
             nextTimeinfo->tm_mon + 1,
             nextTimeinfo->tm_mday,
             nextTimeinfo->tm_hour,
             nextTimeinfo->tm_min,
             nextTimeinfo->tm_sec);
    logWithTimestamp(buffer);
  }
}
```

## Monitoring & Logging

### Fixed-Width Timestamp Format

All log messages use consistent timestamp formatting for readability:

```cpp
void logWithTimestamp(const char* message) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Format: YYYY-MM-DD HH:MM:SS | message
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

### Example Output

```
2026-01-21 09:00:00 | MIST START
2026-01-21 09:00:25 | MIST STOP
2026-01-21 09:00:25 | Next mist: 2026-01-21 11:00:25
2026-01-21 11:00:25 | MIST START
2026-01-21 11:00:50 | MIST STOP
2026-01-21 11:00:50 | Next mist: 2026-01-21 13:00:50
```

## Startup Behavior

### Safety First

```cpp
void setup() {
  Serial.begin(115200);

  // CRITICAL: Ensure relay is OFF on startup
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Changed from HIGH

  // ... WiFi and NTP setup ...
}
```

### Boot Scenarios

| Boot Time | Last Mist State | Behavior |
|-----------|----------------|----------|
| 8:00pm | Unknown | Wait until 9am, then mist immediately |
| 10:00am (first boot) | None (lastMistTime=0) | Mist immediately |
| 10:00am (reboot after 30min) | 9:30am | Wait until 11:30am (2h from 9:30am) |
| 2:00pm (reboot, no stored state) | None (lastMistTime=0) | Mist immediately |

**Note:** Current design does not persist state across reboots. After power loss, device treats itself as "never misted" and will mist immediately upon entering the active window.

## Code Cleanup

### Remove Diagnostic Code

The following diagnostic features will be removed from the main sketch:

- I2C bus scanning (`TB.printI2CBusScan()`)
- WiFi network scanning (`WiFi.scanNetworks()`)
- Loop counter relay testing logic

### Keep Essential Features

- WiFi connection (required for NTP)
- NTP time synchronization (required for scheduling)
- Serial logging (required for monitoring)

### Final loop() Structure

```cpp
void loop() {
  updateMisterStateMachine();

  // Optional: periodic status logging every 5 minutes
  static unsigned long lastStatusDisplay = 0;
  if (millis() - lastStatusDisplay > 300000) {
    logStatus();
    lastStatusDisplay = millis();
  }

  delay(100);
}
```

## Future Enhancements

The design is prepared for future HTTP server integration:

- **State observation**: HTTP endpoints can read `currentState`, `lastMistTime`, `mistStartTime`
- **Manual control**: Add `triggerManualMist()`, `skipNextMist()` functions callable from HTTP handlers
- **Status API**: Add `getNextMistTime()`, `getTimeUntilNextMist()` for UI display
- **Thread safety**: Consider mutexes if using async HTTP server

## Testing Strategy

1. **Relay timing test**: Verify 25-second duration accuracy
2. **Interval test**: Verify 2-hour spacing between mists
3. **Window enforcement**: Verify no misting outside 9am-6pm
4. **Boot behavior**: Test various boot times and verify immediate/delayed misting
5. **Boundary conditions**: Test misting near 6pm boundary
6. **24-hour operation**: Continuous monitoring over full day/night cycle

## Dependencies

- ESP32 Arduino core (WiFi, time.h)
- Adafruit_TestBed (optional, can be removed)
- secrets.h (WiFi credentials and timezone offsets)

## Relay Configuration

- **Pin:** GPIO 13
- **Active state:** HIGH (relay energized, mister ON)
- **Idle state:** LOW (relay off, mister OFF)
- **Startup state:** LOW (safety default)
