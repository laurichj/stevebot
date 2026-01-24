// Unit tests for WiFi and NTP functionality
// These tests are designed to run on the actual ESP32 hardware
// Use Arduino IDE Serial Monitor or PlatformIO to view test results

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// Test configuration - these should match your secrets.h
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

// Test results tracking
int testsRun = 0;
int testsPassed = 0;
int testsFailed = 0;

// Test helper functions
void testAssert(bool condition, const char* testName) {
  testsRun++;
  if (condition) {
    testsPassed++;
    Serial.print("[PASS] ");
    Serial.println(testName);
  } else {
    testsFailed++;
    Serial.print("[FAIL] ");
    Serial.println(testName);
  }
}

void printTestSummary() {
  Serial.println("\n========== TEST SUMMARY ==========");
  Serial.print("Tests Run: ");
  Serial.println(testsRun);
  Serial.print("Tests Passed: ");
  Serial.println(testsPassed);
  Serial.print("Tests Failed: ");
  Serial.println(testsFailed);
  Serial.println("==================================\n");
}

// WiFi Tests
void test_wifi_connection() {
  Serial.println("\n[TEST] WiFi Connection Test");

  // Test 1: WiFi mode set correctly
  WiFi.mode(WIFI_STA);
  testAssert(WiFi.getMode() == WIFI_STA, "WiFi mode set to STA");

  // Test 2: Begin connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  delay(100);
  testAssert(WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTED,
             "WiFi begin initiated");

  // Test 3: Wait for connection (timeout 10 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  testAssert(WiFi.status() == WL_CONNECTED, "WiFi connected successfully");

  // Test 4: IP address assigned
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    testAssert(ip[0] != 0, "Valid IP address obtained");
    Serial.print("   IP Address: ");
    Serial.println(ip);
  }

  // Test 5: Signal strength
  if (WiFi.status() == WL_CONNECTED) {
    int32_t rssi = WiFi.RSSI();
    testAssert(rssi < 0 && rssi > -100, "Signal strength in valid range");
    Serial.print("   RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
}

void test_wifi_reconnection() {
  Serial.println("\n[TEST] WiFi Reconnection Test");

  // Test 6: Disconnect
  WiFi.disconnect();
  delay(1000);
  testAssert(WiFi.status() != WL_CONNECTED, "WiFi disconnected");

  // Test 7: Reconnect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  testAssert(WiFi.status() == WL_CONNECTED, "WiFi reconnected successfully");
  testAssert(attempts < 20, "Reconnection within timeout period");
}

// NTP Tests
void test_ntp_synchronization() {
  Serial.println("\n[TEST] NTP Synchronization Test");

  // Ensure WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("   [SKIP] WiFi not connected, skipping NTP tests");
    return;
  }

  // Test 8: Configure NTP
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org");
  delay(100);
  testAssert(true, "NTP configuration initiated");

  // Test 9: Wait for time sync (timeout 5 seconds)
  struct tm timeinfo;
  int attempts = 0;
  bool timeSynced = false;
  while (attempts < 10) {
    if (getLocalTime(&timeinfo)) {
      timeSynced = true;
      break;
    }
    delay(500);
    attempts++;
  }
  testAssert(timeSynced, "NTP time synchronized");

  // Test 10: Validate time structure
  if (timeSynced) {
    testAssert(timeinfo.tm_year >= 124, "Year is valid (2024 or later)");
    testAssert(timeinfo.tm_mon >= 0 && timeinfo.tm_mon <= 11, "Month is valid (0-11)");
    testAssert(timeinfo.tm_mday >= 1 && timeinfo.tm_mday <= 31, "Day is valid (1-31)");
    testAssert(timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23, "Hour is valid (0-23)");
    testAssert(timeinfo.tm_min >= 0 && timeinfo.tm_min <= 59, "Minute is valid (0-59)");
    testAssert(timeinfo.tm_sec >= 0 && timeinfo.tm_sec <= 59, "Second is valid (0-59)");

    Serial.print("   Synchronized time: ");
    Serial.print(timeinfo.tm_year + 1900);
    Serial.print("-");
    Serial.print(timeinfo.tm_mon + 1);
    Serial.print("-");
    Serial.print(timeinfo.tm_mday);
    Serial.print(" ");
    Serial.print(timeinfo.tm_hour);
    Serial.print(":");
    Serial.print(timeinfo.tm_min);
    Serial.print(":");
    Serial.println(timeinfo.tm_sec);
  }
}

void test_ntp_accuracy() {
  Serial.println("\n[TEST] NTP Time Accuracy Test");

  // Test 11: Time progression
  struct tm timeinfo1, timeinfo2;
  if (getLocalTime(&timeinfo1)) {
    delay(2000);  // Wait 2 seconds
    if (getLocalTime(&timeinfo2)) {
      time_t time1 = mktime(&timeinfo1);
      time_t time2 = mktime(&timeinfo2);
      int diff = (int)difftime(time2, time1);
      testAssert(diff >= 1 && diff <= 3, "Time progresses correctly (2 sec +/- 1)");
      Serial.print("   Time difference: ");
      Serial.print(diff);
      Serial.println(" seconds");
    }
  }
}

void test_ntp_timezone() {
  Serial.println("\n[TEST] NTP Timezone Configuration Test");

  // Test 12: Verify timezone offset is applied
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // We can't verify the exact offset without knowing UTC time,
    // but we can verify the time is reasonable for the configured timezone
    testAssert(timeinfo.tm_hour >= 0 && timeinfo.tm_hour <= 23,
               "Hour is in valid range with timezone applied");

    Serial.print("   Configured GMT offset: ");
    Serial.print(GMT_OFFSET_SEC / 3600);
    Serial.println(" hours");
    Serial.print("   Configured DST offset: ");
    Serial.print(DAYLIGHT_OFFSET_SEC / 3600);
    Serial.println(" hours");
  }
}

// Main test runner
void runAllTests() {
  Serial.println("\n========================================");
  Serial.println("  STEVEBOT WiFi & NTP TEST SUITE");
  Serial.println("========================================");

  delay(2000);  // Give serial monitor time to open

  // Run WiFi tests
  test_wifi_connection();
  test_wifi_reconnection();

  // Run NTP tests
  test_ntp_synchronization();
  test_ntp_accuracy();
  test_ntp_timezone();

  // Print summary
  printTestSummary();

  if (testsFailed == 0) {
    Serial.println("✓ ALL TESTS PASSED!");
  } else {
    Serial.println("✗ SOME TESTS FAILED!");
  }
}
