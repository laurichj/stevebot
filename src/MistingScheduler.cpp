// src/MistingScheduler.cpp
#include "MistingScheduler.h"

MistingScheduler::MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController)
    : timeProvider(timeProvider), relayController(relayController),
      currentState(WAITING_SYNC), lastMistTime(0), mistStartTime(0) {
}

void MistingScheduler::update() {
    switch (currentState) {
        case WAITING_SYNC:
            {
                struct tm timeinfo;
                if (timeProvider->getTime(&timeinfo)) {
                    currentState = IDLE;
                    lastMistTime = 0;
                    // Fall through to check IDLE conditions immediately
                    [[fallthrough]];
                } else {
                    break;
                }
            }

        case IDLE:
            if (shouldStartMisting()) {
                startMisting();
            }
            break;

        case MISTING:
            {
                unsigned long elapsed = timeProvider->getMillis() - mistStartTime;
                if (elapsed >= MIST_DURATION) {
                    stopMisting();
                }
            }
            break;
    }
}

bool MistingScheduler::isInActiveWindow() {
    struct tm timeinfo;
    if (!timeProvider->getTime(&timeinfo)) {
        return false;
    }

    int hour = timeinfo.tm_hour;
    return (hour >= ACTIVE_WINDOW_START && hour < ACTIVE_WINDOW_END);
}

bool MistingScheduler::shouldStartMisting() {
    if (!isInActiveWindow()) return false;
    if (currentState != IDLE) return false;

    // First mist
    if (lastMistTime == 0) return true;

    // Check if 2 hours have passed
    unsigned long elapsed = timeProvider->getMillis() - lastMistTime;
    return (elapsed >= MIST_INTERVAL);
}

void MistingScheduler::startMisting() {
    relayController->turnOn();
    mistStartTime = timeProvider->getMillis();
    lastMistTime = timeProvider->getMillis();
    currentState = MISTING;
}

void MistingScheduler::stopMisting() {
    relayController->turnOff();
    currentState = IDLE;
}
