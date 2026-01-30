// src/MistingScheduler.h
#ifndef MISTING_SCHEDULER_H
#define MISTING_SCHEDULER_H

#include "ITimeProvider.h"
#include "IRelayController.h"
#include "IStateStorage.h"

// Logging callback type
typedef void (*LogCallback)(const char* message);

enum MisterState {
  WAITING_SYNC,   // Time not yet synchronized
  IDLE,           // Waiting for next misting time
  MISTING         // Actively running the mister
};

class MistingScheduler {
public:
    MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, IStateStorage* stateStorage = nullptr, LogCallback logger = nullptr);

    // Call from main loop
    void update();

    // Query current state
    MisterState getState() const { return currentState; }
    time_t getLastMistEpoch() const { return lastMistEpoch; }
    unsigned long getMistStartTime() const { return mistStartTime; }

    // State management
    void loadState();
    void saveState();
    void setEnabled(bool enabled);
    bool isEnabled() const { return schedulerEnabled; }

    // Manual control
    void forceMist();
    void printStatus();

    // Configuration
    static const unsigned long MIST_DURATION = 25000;         // 25 seconds
    static const unsigned long MIST_INTERVAL_SECONDS = 7200;  // 2 hours in seconds
    static const int ACTIVE_WINDOW_START = 9;                 // 9am
    static const int ACTIVE_WINDOW_END = 18;                  // 6pm (exclusive)

private:
    ITimeProvider* timeProvider;
    IRelayController* relayController;
    IStateStorage* stateStorage;
    LogCallback logger;

    MisterState currentState;
    time_t lastMistEpoch;         // Epoch time of last mist start (seconds)
    time_t lastKnownEpoch;        // Track last known epoch for time jump detection
    unsigned long mistStartTime;  // millis() when mist started (for duration)
    bool hasEverMisted;
    bool schedulerEnabled;

    // Internal logic methods
    bool isInActiveWindow();
    bool shouldStartMisting();
    void startMisting();
    void stopMisting();
    void log(const char* message);
};

#endif
