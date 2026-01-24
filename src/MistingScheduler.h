// src/MistingScheduler.h
#ifndef MISTING_SCHEDULER_H
#define MISTING_SCHEDULER_H

#include "ITimeProvider.h"
#include "IRelayController.h"

// Logging callback type
typedef void (*LogCallback)(const char* message);

enum MisterState {
  WAITING_SYNC,   // Time not yet synchronized
  IDLE,           // Waiting for next misting time
  MISTING         // Actively running the mister
};

class MistingScheduler {
public:
    MistingScheduler(ITimeProvider* timeProvider, IRelayController* relayController, LogCallback logger = nullptr);

    // Call from main loop
    void update();

    // Query current state
    MisterState getState() const { return currentState; }
    unsigned long getLastMistTime() const { return lastMistTime; }
    unsigned long getMistStartTime() const { return mistStartTime; }

    // Configuration
    static const unsigned long MIST_DURATION = 25000;      // 25 seconds
    static const unsigned long MIST_INTERVAL = 7200000;    // 2 hours
    static const int ACTIVE_WINDOW_START = 9;              // 9am
    static const int ACTIVE_WINDOW_END = 18;               // 6pm (exclusive)

private:
    ITimeProvider* timeProvider;
    IRelayController* relayController;
    LogCallback logger;

    MisterState currentState;
    unsigned long lastMistTime;
    unsigned long mistStartTime;
    bool hasEverMisted;

    // Internal logic methods
    bool isInActiveWindow();
    bool shouldStartMisting();
    void startMisting();
    void stopMisting();
    void log(const char* message);
};

#endif
