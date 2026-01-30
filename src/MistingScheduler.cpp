// src/MistingScheduler.cpp
#include "MistingScheduler.h"
#include <stdio.h>

MistingScheduler::MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, IStateStorage* stateStorage, LogCallback logger)
    : timeProvider(timeProvider), relayController(relayController), stateStorage(stateStorage), logger(logger),
      currentState(WAITING_SYNC), lastMistEpoch(0), lastKnownEpoch(0), mistStartTime(0), hasEverMisted(false), schedulerEnabled(true) {
}

void MistingScheduler::update() {
    // Check if scheduler is disabled
    if (!schedulerEnabled) {
        return;
    }

    // Time jump detection (NTP adjustments)
    time_t currentEpoch = timeProvider->getEpochTime();
    if (lastKnownEpoch > 0 && currentEpoch > 0) {
        time_t timeDelta = (currentEpoch > lastKnownEpoch) ?
                           (currentEpoch - lastKnownEpoch) :
                           (lastKnownEpoch - currentEpoch);

        // If time jumped more than 5 minutes, log it
        if (timeDelta > 300) {
            char buffer[80];
            snprintf(buffer, sizeof(buffer),
                     "WARNING: Time jump detected: %ld seconds", (long)timeDelta);
            log(buffer);
        }
    }
    lastKnownEpoch = currentEpoch;

    switch (currentState) {
        case WAITING_SYNC:
            {
                struct tm timeinfo;
                if (timeProvider->getTime(&timeinfo)) {
                    currentState = IDLE;
                    // Don't reset lastMistEpoch - it may have been loaded from storage
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
                } else if (elapsed >= MIST_DURATION * 3) {
                    // Safety failsafe: mist duration exceeded 3x normal time (75 seconds)
                    log("CRITICAL: Mist duration exceeded safety limit, forcing stop");
                    relayController->turnOff();
                    currentState = IDLE;
                    // Don't save state or update lastMistEpoch - this is an error condition
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

    // Check if 2 hours have passed using epoch time
    time_t currentEpoch = timeProvider->getEpochTime();
    if (currentEpoch == 0 || lastMistEpoch == 0) {
        return false;  // Time not available
    }

    time_t elapsed = currentEpoch - lastMistEpoch;
    return (elapsed >= MIST_INTERVAL_SECONDS);
}

void MistingScheduler::startMisting() {
    relayController->turnOn();
    mistStartTime = timeProvider->getMillis();
    lastMistEpoch = timeProvider->getEpochTime();
    currentState = MISTING;
    hasEverMisted = true;
    log("MIST START");
    // Don't save here - save only on successful completion (reduces NVS writes)
}

void MistingScheduler::stopMisting() {
    relayController->turnOff();
    currentState = IDLE;
    log("MIST STOP");
    // Save state after successful misting cycle (single write per cycle)
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

    // Note: NVS still stores as unsigned long for compatibility
    // We cast it to time_t (which should also be compatible)
    lastMistEpoch = (time_t)stateStorage->getLastMistTime();
    hasEverMisted = stateStorage->getHasEverMisted();
    schedulerEnabled = stateStorage->getEnabled();

    if (lastMistEpoch > 0) {
        log("Loaded state from NVS");
    }
}

void MistingScheduler::saveState() {
    if (!stateStorage) {
        return;
    }

    // Save epoch time as unsigned long for NVS compatibility
    stateStorage->save((unsigned long)lastMistEpoch, hasEverMisted, schedulerEnabled);
}

void MistingScheduler::setEnabled(bool enabled) {
    schedulerEnabled = enabled;

    // If enabling and stuck in WAITING_SYNC, check if time is now available
    if (enabled && currentState == WAITING_SYNC) {
        struct tm timeinfo;
        if (timeProvider->getTime(&timeinfo)) {
            currentState = IDLE;
            lastMistEpoch = 0;
            log("Scheduler ENABLED (transitioned to IDLE)");
        } else {
            log("Scheduler ENABLED (waiting for time sync)");
        }
    } else if (enabled) {
        log("Scheduler ENABLED");
    } else {
        log("Scheduler DISABLED");
    }

    saveState();
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

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "STATUS: state=%s enabled=%s hasEverMisted=%s",
             stateStr,
             schedulerEnabled ? "true" : "false",
             hasEverMisted ? "true" : "false");
    log(buffer);

    // Print last mist time using epoch time
    if (hasEverMisted && lastMistEpoch > 0) {
        time_t currentEpoch = timeProvider->getEpochTime();
        if (currentEpoch > 0) {
            time_t elapsed = currentEpoch - lastMistEpoch;
            long elapsedMin = elapsed / 60;
            long elapsedHours = elapsedMin / 60;

            snprintf(buffer, sizeof(buffer), "STATUS: lastMist=%ldh %ldm ago",
                     elapsedHours, elapsedMin % 60);
            log(buffer);
        }
    } else {
        log("STATUS: lastMist=never");
    }

    // Print next mist estimate if in IDLE state
    if (currentState == IDLE && schedulerEnabled && hasEverMisted) {
        time_t currentEpoch = timeProvider->getEpochTime();
        if (currentEpoch > 0 && lastMistEpoch > 0) {
            time_t elapsed = currentEpoch - lastMistEpoch;
            if (elapsed < MIST_INTERVAL_SECONDS) {
                time_t remaining = MIST_INTERVAL_SECONDS - elapsed;
                long remainingMin = remaining / 60;
                long remainingHours = remainingMin / 60;

                snprintf(buffer, sizeof(buffer), "STATUS: nextMist=in %ldh %ldm",
                         remainingHours, remainingMin % 60);
                log(buffer);
            } else {
                log("STATUS: nextMist=waiting for active window");
            }
        }
    }
}
