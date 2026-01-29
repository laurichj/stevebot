// src/MistingScheduler.cpp
#include "MistingScheduler.h"
#include <stdio.h>

MistingScheduler::MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, IStateStorage* stateStorage, LogCallback logger)
    : timeProvider(timeProvider), relayController(relayController), stateStorage(stateStorage), logger(logger),
      currentState(WAITING_SYNC), lastMistTime(0), mistStartTime(0), hasEverMisted(false), schedulerEnabled(true) {
}

void MistingScheduler::update() {
    // Check if scheduler is disabled
    if (!schedulerEnabled) {
        return;
    }

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
    if (!hasEverMisted) return true;

    // Check if 2 hours have passed
    unsigned long elapsed = timeProvider->getMillis() - lastMistTime;
    return (elapsed >= MIST_INTERVAL);
}

void MistingScheduler::startMisting() {
    relayController->turnOn();
    mistStartTime = timeProvider->getMillis();
    lastMistTime = timeProvider->getMillis();
    currentState = MISTING;
    hasEverMisted = true;
    log("MIST START");
    saveState();
}

void MistingScheduler::stopMisting() {
    relayController->turnOff();
    currentState = IDLE;
    log("MIST STOP");
    saveState();
}

void MistingScheduler::log(const char* message) {
    if (logger) {
        logger(message);
    }
}

void MistingScheduler::loadState() {
    if (!stateStorage) {
        return;
    }

    lastMistTime = stateStorage->getLastMistTime();
    hasEverMisted = stateStorage->getHasEverMisted();
    schedulerEnabled = stateStorage->getEnabled();

    if (lastMistTime > 0) {
        log("Loaded state from NVS");
    }
}

void MistingScheduler::saveState() {
    if (!stateStorage) {
        return;
    }

    stateStorage->save(lastMistTime, hasEverMisted, schedulerEnabled);
}

void MistingScheduler::setEnabled(bool enabled) {
    schedulerEnabled = enabled;
    saveState();

    if (enabled) {
        log("Scheduler ENABLED");
    } else {
        log("Scheduler DISABLED");
    }
}

void MistingScheduler::forceMist() {
    // Check if already misting
    if (currentState == MISTING) {
        log("ERROR: Already misting, cannot force");
        return;
    }

    // Check if scheduler is enabled
    if (!schedulerEnabled) {
        log("ERROR: Scheduler disabled, cannot force mist");
        return;
    }

    log("FORCE MIST");
    startMisting();
}

void MistingScheduler::printStatus() {
    // Print current state
    const char* stateStr = "";
    switch (currentState) {
        case WAITING_SYNC:
            stateStr = "WAITING_SYNC";
            break;
        case IDLE:
            stateStr = "IDLE";
            break;
        case MISTING:
            stateStr = "MISTING";
            break;
    }

    char buffer[200];
    snprintf(buffer, sizeof(buffer), "STATUS: state=%s enabled=%s hasEverMisted=%s",
             stateStr,
             schedulerEnabled ? "true" : "false",
             hasEverMisted ? "true" : "false");
    log(buffer);

    // Print last mist time
    if (hasEverMisted && lastMistTime > 0) {
        struct tm timeinfo;
        if (timeProvider->getTime(&timeinfo)) {
            unsigned long elapsed = timeProvider->getMillis() - lastMistTime;
            unsigned long elapsedSec = elapsed / 1000;
            unsigned long elapsedMin = elapsedSec / 60;
            unsigned long elapsedHours = elapsedMin / 60;

            snprintf(buffer, sizeof(buffer), "STATUS: lastMist=%luh %lum ago",
                     elapsedHours, elapsedMin % 60);
            log(buffer);
        }
    } else {
        log("STATUS: lastMist=never");
    }

    // Print next mist estimate if in IDLE state
    if (currentState == IDLE && schedulerEnabled && hasEverMisted) {
        unsigned long elapsed = timeProvider->getMillis() - lastMistTime;
        if (elapsed < MIST_INTERVAL) {
            unsigned long remaining = MIST_INTERVAL - elapsed;
            unsigned long remainingSec = remaining / 1000;
            unsigned long remainingMin = remainingSec / 60;
            unsigned long remainingHours = remainingMin / 60;

            snprintf(buffer, sizeof(buffer), "STATUS: nextMist=in %luh %lum",
                     remainingHours, remainingMin % 60);
            log(buffer);
        } else {
            log("STATUS: nextMist=waiting for active window");
        }
    }
}
