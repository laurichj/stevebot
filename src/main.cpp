// src/main.cpp
#include <Arduino.h>
#include "WiFi.h"
#include "secrets.h"
#include "MistingScheduler.h"
#include "NTPTimeProvider.h"
#include "GPIORelayController.h"
#include "NVSStateStorage.h"
#include <esp_task_wdt.h>

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

    // Relay safety check FIRST: ensure relay is OFF on boot (before watchdog init)
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    delay(50);  // Allow relay time to disengage

    // Verify relay state (should read LOW)
    if (digitalRead(RELAY_PIN) == HIGH) {
        Serial.println("CRITICAL: Relay stuck HIGH after reset!");
        // Attempt one more time to turn it off
        digitalWrite(RELAY_PIN, LOW);
        delay(50);
    }

    Serial.println("Relay initialized and verified OFF");

    // WiFi setup (BEFORE watchdog init to avoid timeout during slow WiFi)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
  
	// Old esp32 stuff
	WiFi.setMinSecurity(WIFI_AUTH_WEP);
	WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
	WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

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

    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


    // Wait for connection (no watchdog yet, so safe to block)
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

        // Initialize NTP time synchronization with timezone support
        Serial.println("Synchronizing time with NTP server...");
#ifdef TIMEZONE_STRING
        // Use POSIX timezone string (handles DST automatically)
        configTzTime(TIMEZONE_STRING, ntpServer);
#else
        // Fallback to legacy GMT offset method
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);
#endif

        // Wait for time to be set (still no watchdog, safe to block)
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

    // NOW initialize watchdog timer AFTER all blocking operations
    // (10 second timeout, trigger panic/reset)
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);  // Add current task to watchdog

    // Check if system was reset by watchdog
    esp_reset_reason_t resetReason = esp_reset_reason();
    if (resetReason == ESP_RST_TASK_WDT) {
        logWithTimestamp("WARNING: System restarted due to watchdog timeout");
    }

    logWithTimestamp("Setup complete, entering main loop");
}

void processSerialCommands() {
    // Non-blocking: only process if data is available
    if (!Serial.available()) {
        return;
    }

    // Use fixed buffer instead of String to avoid heap fragmentation
    const size_t MAX_CMD_LEN = 32;
    char cmdBuffer[MAX_CMD_LEN];
    size_t idx = 0;

    // Read command with bounds checking
    while (Serial.available() && idx < MAX_CMD_LEN - 1) {
        int c = Serial.read();
        if (c == -1) break;  // No data
        if (c == '\n' || c == '\r') break;  // End of command
        cmdBuffer[idx++] = (char)c;
    }
    cmdBuffer[idx] = '\0';

    // Flush any remaining characters if command was too long
    if (idx == MAX_CMD_LEN - 1) {
        while (Serial.available() && Serial.read() != '\n');
        Serial.println("ERROR: Command too long (max 31 chars)");
        return;
    }

    // Trim trailing whitespace
    while (idx > 0 && (cmdBuffer[idx-1] == ' ' || cmdBuffer[idx-1] == '\t')) {
        cmdBuffer[--idx] = '\0';
    }

    // Trim leading whitespace
    char* cmd = cmdBuffer;
    while (*cmd == ' ' || *cmd == '\t') {
        cmd++;
    }

    // Convert to uppercase in-place
    for (char* p = cmd; *p; p++) {
        if (*p >= 'a' && *p <= 'z') {
            *p = *p - 32;
        }
    }

    // Skip empty commands
    if (*cmd == '\0') {
        return;
    }

    // Process commands using strcmp for safety
    if (strcmp(cmd, "ENABLE") == 0) {
        scheduler.setEnabled(true);
        Serial.println("OK: Scheduler enabled");
    } else if (strcmp(cmd, "DISABLE") == 0) {
        scheduler.setEnabled(false);
        Serial.println("OK: Scheduler disabled");
    } else if (strcmp(cmd, "FORCE_MIST") == 0) {
        scheduler.forceMist();
        Serial.println("OK: Force mist command sent");
    } else if (strcmp(cmd, "STATUS") == 0) {
        scheduler.printStatus();
    } else {
        Serial.print("ERROR: Unknown command: ");
        Serial.println(cmd);
    }
}

// WiFi monitoring
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 60000;  // Check every 1 minute

void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        logWithTimestamp("WARNING: WiFi disconnected, attempting reconnect");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            esp_task_wdt_reset();  // Feed watchdog during reconnection
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            logWithTimestamp("WiFi reconnected");
            // Force NTP resync after reconnection
#ifdef TIMEZONE_STRING
            configTzTime(TIMEZONE_STRING, ntpServer);
#else
            configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);
#endif
        } else {
            logWithTimestamp("ERROR: WiFi reconnection failed");
        }
    }
}

void loop() {
    esp_task_wdt_reset();  // Feed the watchdog to prove system is alive

    // Periodic WiFi connection check
    if (millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        checkWiFiConnection();
        lastWiFiCheck = millis();
    }

    processSerialCommands();
    scheduler.update();
    delay(100);
}
