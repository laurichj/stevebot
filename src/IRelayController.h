// src/IRelayController.h
#ifndef I_RELAY_CONTROLLER_H
#define I_RELAY_CONTROLLER_H

class IRelayController {
public:
    virtual ~IRelayController() = default;

    // Turn relay on (activate mister)
    virtual void turnOn() = 0;

    // Turn relay off (deactivate mister)
    virtual void turnOff() = 0;
};

#endif
