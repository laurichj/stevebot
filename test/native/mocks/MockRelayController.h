// test/native/mocks/MockRelayController.h
#ifndef MOCK_RELAY_CONTROLLER_H
#define MOCK_RELAY_CONTROLLER_H

#include "IRelayController.h"

class MockRelayController : public IRelayController {
public:
    MockRelayController() : isOn(false), turnOnCount(0), turnOffCount(0) {}

    void turnOn() override {
        isOn = true;
        turnOnCount++;
    }

    void turnOff() override {
        isOn = false;
        turnOffCount++;
    }

    // Test inspection methods
    bool getIsOn() const { return isOn; }
    int getTurnOnCount() const { return turnOnCount; }
    int getTurnOffCount() const { return turnOffCount; }
    void reset() { isOn = false; turnOnCount = 0; turnOffCount = 0; }

private:
    bool isOn;
    int turnOnCount;
    int turnOffCount;
};

#endif
