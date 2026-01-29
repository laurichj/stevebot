// src/main.cpp
#include <Arduino.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"
#include "NVSStateStorage.h"

#define RELAY_PIN 13

// NTP server configuration
const char* ntpServer = "pool.ntp.org";

// Logging function with timestamp
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

// Global instances
NTPTimeProvider timeProvider;
GPIORelayController relayController(RELAY_PIN);
NVSStateStorage stateStorage(logWithTimestamp);
MistingScheduler scheduler(&timeProvider, &relayController, &stateStorage, logWithTimestamp);

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

            // Load state from NVS after time is synchronized
            scheduler.loadState();
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
