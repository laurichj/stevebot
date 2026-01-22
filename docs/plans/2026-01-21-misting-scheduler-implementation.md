# Misting Scheduler Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement automated misting scheduler that activates relay for 25 seconds every 2 hours during daylight hours (9am-6pm).

**Architecture:** State machine with three states (WAITING_SYNC, IDLE, MISTING) manages misting lifecycle. Time window enforcement uses NTP-synchronized time. All state transitions are logged with fixed-width timestamps.

**Tech Stack:** ESP32 Arduino, WiFi/NTP for time sync, GPIO for relay control

---

## Task 1: Add State Machine Types and Constants

**Files:**
- Modify: `stevebot.ino` (add after includes, before existing code)

**Step 1: Add state enum and constants**

Add after line 12 (`#define RELAY_PIN 13`):

```cpp
// Misting state machine
enum MisterState {
  WAITING_SYNC,   // Time not yet synchronized, can't schedule
  IDLE,           // Waiting for next misting time
  MISTING         // Actively running the mister
};

// Misting configuration
const unsigned long MIST_DURATION = 25000;      // 25 seconds in milliseconds
const unsigned long MIST_INTERVAL = 7200000;    // 2 hours in milliseconds

// State variables
MisterState currentState = WAITING_SYNC;
unsigned long lastMistTime = 0;        // millis() when last mist started
unsigned long mistStartTime = 0;       // millis() when current mist started
```

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS (compiles with no errors)

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Add state machine types and constants for misting scheduler"
```

---

## Task 2: Add Time Window Check Function

**Files:**
- Modify: `stevebot.ino` (add before `setup()`)

**Step 1: Add isInActiveWindow function**

Add before the `setup()` function (around line 21):

```cpp
// Check if current time is within active misting window (9am-6pm)
bool isInActiveWindow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;  // Can't determine time, not safe to mist
  }

  int hour = timeinfo.tm_hour;
  return (hour >= 9 && hour < 18);  // 9am to 5:59:59pm
}
```

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Add time window check function for 9am-6pm enforcement"
```

---

## Task 3: Add Timestamp Logging Function

**Files:**
- Modify: `stevebot.ino` (add before `setup()`)

**Step 1: Add logWithTimestamp function**

Add before the `setup()` function:

```cpp
// Log message with fixed-width timestamp
void logWithTimestamp(const char* message) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Fixed-width format: YYYY-MM-DD HH:MM:SS | message
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

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Add fixed-width timestamp logging function"
```

---

## Task 4: Add Start and Stop Misting Functions

**Files:**
- Modify: `stevebot.ino` (add before `setup()`)

**Step 1: Add startMisting function**

Add before the `setup()` function:

```cpp
// Start misting cycle
void startMisting() {
  digitalWrite(RELAY_PIN, HIGH);
  mistStartTime = millis();
  lastMistTime = millis();
  currentState = MISTING;
  logWithTimestamp("MIST START");
}
```

**Step 2: Add stopMisting function**

Add after `startMisting()`:

```cpp
// Stop misting cycle and log next scheduled time
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

**Step 3: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 4: Commit**

```bash
git add stevebot.ino
git commit -m "Add start and stop misting functions with logging"
```

---

## Task 5: Add Should-Mist Check Function

**Files:**
- Modify: `stevebot.ino` (add before `setup()`)

**Step 1: Add shouldStartMisting function**

Add before the `setup()` function:

```cpp
// Check if conditions are met to start misting
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

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Add should-mist condition check function"
```

---

## Task 6: Add State Machine Update Function

**Files:**
- Modify: `stevebot.ino` (add before `setup()`)

**Step 1: Add updateMisterStateMachine function**

Add before the `setup()` function:

```cpp
// Main state machine update - call from loop()
void updateMisterStateMachine() {
  switch (currentState) {
    case WAITING_SYNC:
      if (timeInitialized) {
        currentState = IDLE;
        lastMistTime = 0;  // Treat as never misted
        logWithTimestamp("Time synchronized, scheduler active");
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
```

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Add state machine update function"
```

---

## Task 7: Fix Relay Startup Safety

**Files:**
- Modify: `stevebot.ino:26` (in `setup()` function)

**Step 1: Change relay initialization to LOW**

Find this line in `setup()` (line 26):

```cpp
  digitalWrite(RELAY_PIN, HIGH);
```

Replace with:

```cpp
  digitalWrite(RELAY_PIN, LOW);  // Safety: ensure mister is OFF on startup
```

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Fix relay startup state to OFF for safety"
```

---

## Task 8: Remove Diagnostic Code from Loop

**Files:**
- Modify: `stevebot.ino` (in `loop()` function, lines 92-125)

**Step 1: Remove I2C and WiFi scanning code**

Find and remove these lines from `loop()` (the entire `if (loopCounter == 0)` block and the `else if(loopCounter == 128)` block):

```cpp
  if (loopCounter == 0) {
    // Test I2C!
    Serial.print("I2C port ");
    TB.theWire = &Wire;
    TB.printI2CBusScan();

    // Test WiFi Scan!
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.print("WiFi AP scan done...");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
      }
    }
    Serial.println("");

    // turn the relay pin on
    digitalWrite(RELAY_PIN, HIGH);
  } else if(loopCounter == 128) {
    digitalWrite(RELAY_PIN, LOW);
  }
```

**Step 2: Remove loopCounter variable**

Find and remove this line before `loop()` (line 79):

```cpp
uint8_t loopCounter = 0;
```

**Step 3: Remove loopCounter increment**

Find and remove this line at end of `loop()` (line 128):

```cpp
  loopCounter++;
```

**Step 4: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 5: Commit**

```bash
git add stevebot.ino
git commit -m "Remove diagnostic I2C and WiFi scanning code from loop"
```

---

## Task 9: Add State Machine to Loop

**Files:**
- Modify: `stevebot.ino` (in `loop()` function)

**Step 1: Add state machine call to loop**

The `loop()` function should now look like this (replace the existing loop after removing diagnostic code):

```cpp
// the loop routine runs over and over again forever:
void loop() {
  // Update misting state machine
  updateMisterStateMachine();

  // Periodic time display for testing/debugging
  static unsigned long lastTimeDisplay = 0;
  if (timeInitialized && (millis() - lastTimeDisplay > 10000)) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.print("Current time: ");
      Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
    lastTimeDisplay = millis();
  }

  delay(100);
}
```

**Step 2: Verify compilation**

```bash
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

Expected: SUCCESS

**Step 3: Commit**

```bash
git add stevebot.ino
git commit -m "Integrate state machine into main loop"
```

---

## Task 10: Update Documentation

**Files:**
- Modify: `README.md` (update TODO section)

**Step 1: Update README TODO section**

Find the "Core Functionality" section in README.md and update checkboxes:

```markdown
### Core Functionality
- [x] Add WiFi credentials configuration (SSID and password)
- [x] Implement NTP client for time synchronization
- [x] Add timezone configuration support
- [x] Implement scheduling logic (9am-6pm, every 2 hours)
- [x] Implement 25-second mister activation timer
- [x] Remove existing I2C and WiFi scanning diagnostic code
```

**Step 2: Commit**

```bash
git add README.md
git commit -m "Update README to reflect completed core functionality"
```

---

## Task 11: Manual Testing

**Files:**
- Test: `stevebot.ino` (upload and monitor)

**Step 1: Upload to hardware**

```bash
# Adjust port as needed for your hardware
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
```

**Step 2: Open serial monitor**

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

**Step 3: Verify expected behavior**

Watch serial output and verify:

1. **WiFi connection:** Device connects and gets IP
2. **Time sync:** NTP synchronization succeeds
3. **State transition:** See "Time synchronized, scheduler active"
4. **Window check:** If currently 9am-6pm, should see misting behavior
5. **Timestamp format:** All logs use fixed-width format: `YYYY-MM-DD HH:MM:SS | message`
6. **Misting cycle:** If in window and first boot, should see:
   - `MIST START`
   - (25 seconds later) `MIST STOP`
   - `Next mist: YYYY-MM-DD HH:MM:SS`

**Step 4: Document test results**

Create a test log file with observations:

```bash
# In the worktree
cat > test-log.txt << 'EOF'
Manual Test Results - YYYY-MM-DD

Test Time: [current time]
Within Active Window: [yes/no]

Observed Behavior:
- WiFi connection: [success/fail]
- NTP sync: [success/fail]
- State transition to IDLE: [success/fail]
- Misting triggered: [yes/no - expected based on window]
- Relay activated: [verify physically if possible]
- Timestamp format: [correct/incorrect]
- Mist duration: [measured time]
- Next mist calculation: [displayed time]

Notes:
[Any observations or issues]
EOF
```

Fill in the test log with actual results.

**Step 5: Commit test log**

```bash
git add test-log.txt
git commit -m "Add manual testing log for misting scheduler"
```

---

## Completion Checklist

- [ ] All code compiles without errors
- [ ] Relay initializes to LOW (OFF) on startup
- [ ] State machine transitions correctly
- [ ] Time window enforcement works (9am-6pm only)
- [ ] Misting duration is 25 seconds
- [ ] Interval between mists is 2 hours
- [ ] Fixed-width timestamp logging works
- [ ] Next mist time is calculated and logged
- [ ] Diagnostic code removed from loop
- [ ] README updated
- [ ] Manual testing completed and documented

---

## Next Steps After Implementation

1. **Use @superpowers:verification-before-completion** to verify all functionality
2. **Use @superpowers:finishing-a-development-branch** to decide merge/PR/cleanup strategy
3. Consider implementing safety features from README TODO:
   - Watchdog timer
   - Manual override via serial commands
   - Non-volatile storage for last mist time
