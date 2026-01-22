// SPDX-FileCopyrightText: 2022 Limor Fried for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "WiFi.h"
#include <Adafruit_TestBed.h>
#include "secrets.h"
#include "time.h"

extern Adafruit_TestBed TB;

#define RELAY_PIN 13

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

// NTP server configuration
const char* ntpServer = "pool.ntp.org";

// Time tracking
bool timeInitialized = false;

// Check if current time is within active misting window (9am-6pm)
bool isInActiveWindow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false;  // Can't determine time, not safe to mist
  }

  int hour = timeinfo.tm_hour;
  return (hour >= 9 && hour < 18);  // 9am to 5:59:59pm
}

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

// Start misting cycle
void startMisting() {
  digitalWrite(RELAY_PIN, HIGH);
  mistStartTime = millis();
  lastMistTime = millis();
  currentState = MISTING;
  logWithTimestamp("MIST START");
}

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

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);

  // enable the relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Safety: ensure mister is OFF on startup

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
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
      timeInitialized = true;
    } else {
      Serial.println("\nFailed to synchronize time!");
      timeInitialized = false;
    }
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

// the loop routine runs over and over again forever:
uint8_t loopCounter = 0;
void loop() {
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

  delay(100);
  loopCounter++;
}
