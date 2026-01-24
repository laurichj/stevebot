// src/GPIORelayController.h
#ifndef GPIO_RELAY_CONTROLLER_H
#define GPIO_RELAY_CONTROLLER_H

#include "IRelayController.h"
#include <Arduino.h>

class GPIORelayController : public IRelayController {
public:
    GPIORelayController(int pin) : relayPin(pin) {
        pinMode(relayPin, OUTPUT);
        digitalWrite(relayPin, LOW);  // Safety: OFF on construction
    }

    void turnOn() override {
        digitalWrite(relayPin, HIGH);
    }

    void turnOff() override {
        digitalWrite(relayPin, LOW);
    }

private:
    int relayPin;
};

#endif
