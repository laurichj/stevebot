# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commit Guidelines

**IMPORTANT**: Do NOT add "Co-Authored-By: Claude" or any Claude Code attribution text to commit messages unless explicitly instructed by the user.

## Project Overview

This is an Arduino sketch for ESP32-based hardware testing and diagnostics, specifically designed for Adafruit development boards. The project name is "stevebot".

## Hardware Requirements

- ESP32 microcontroller (Adafruit board)
- Relay connected to pin 13
- I2C devices (optional, for testing)
- WiFi capability (built into ESP32)

## Dependencies

- **Adafruit_TestBed library**: Required for I2C bus scanning functionality
- **WiFi.h**: Built-in ESP32 WiFi library

## Building and Uploading

This is an Arduino sketch (.ino file) that must be compiled and uploaded using the Arduino IDE or Arduino CLI:

- **Arduino IDE**: Open `stevebot.ino` and use the Upload button
- **Arduino CLI**:
  ```bash
  arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
  arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:adafruit_feather_esp32s2 stevebot.ino
  ```
  (Adjust port and board FQBN as needed for your specific hardware)

## Running Tests

Unit tests for WiFi and NTP functionality are located in the `test/` directory:

```bash
# Compile and upload tests
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s2 test/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:adafruit_feather_esp32s2 test/
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

See `test/README.md` for detailed test documentation.

## Code Architecture

Single-file Arduino sketch with standard structure:
- `setup()`: Initializes serial communication, relay pin, and WiFi in station mode
- `loop()`: Periodically performs I2C scanning, WiFi network scanning, and relay control
- Uses a loop counter to trigger diagnostics on first iteration and control relay timing

## Serial Communication

The sketch outputs diagnostic information to serial at 115200 baud. Connect via serial monitor to view:
- I2C device scan results
- WiFi network scan results (SSID, RSSI, encryption status)
