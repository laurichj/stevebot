// src/main.cpp
#include <Arduino.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"

#define RELAY_PIN 13

// NTP server configuration
const char* ntpServer = "pool.ntp.org";

// Global instances
NTPTimeProvider timeProvider;
GPIORelayController relayController(RELAY_PIN);
MistingScheduler scheduler(&timeProvider, &relayController);

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
