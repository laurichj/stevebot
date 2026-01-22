// SPDX-FileCopyrightText: 2022 Limor Fried for Adafruit Industries
//
// SPDX-License-Identifier: MIT
//
// Test runner for Stevebot WiFi and NTP functionality
// Upload this sketch to run the test suite

#include "secrets.h"

// Declare test runner function
void runAllTests();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("  Starting Stevebot Test Suite");
  Serial.println("========================================");

  // Run all tests
  runAllTests();

  Serial.println("\nTests complete. System will halt.");
}

void loop() {
  // Tests run once in setup, then halt
  delay(1000);
}
