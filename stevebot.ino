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

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);

  // enable the relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

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
