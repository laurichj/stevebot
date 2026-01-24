// test/native/mocks/MockTimeProvider.h
#ifndef MOCK_TIME_PROVIDER_H
#define MOCK_TIME_PROVIDER_H

#include "ITimeProvider.h"
#include <cstring>

class MockTimeProvider : public ITimeProvider {
public:
    MockTimeProvider() : timeAvailable(true), currentMillis(0) {
        memset(&mockTime, 0, sizeof(mockTime));
        // Default to a valid time
        mockTime.tm_year = 126;  // 2026
        mockTime.tm_mon = 0;     // January
        mockTime.tm_mday = 22;
        mockTime.tm_hour = 10;   // 10am (in active window)
        mockTime.tm_min = 0;
        mockTime.tm_sec = 0;
    }

    bool getTime(struct tm* timeinfo) override {
        if (!timeAvailable) return false;
        *timeinfo = mockTime;
        return true;
    }

    unsigned long getMillis() override {
        return currentMillis;
    }

    // Test control methods
    void setHour(int hour) { mockTime.tm_hour = hour; }
    void setTimeAvailable(bool available) { timeAvailable = available; }
    void advanceMillis(unsigned long ms) { currentMillis += ms; }
    void setMillis(unsigned long ms) { currentMillis = ms; }

private:
    struct tm mockTime;
    bool timeAvailable;
    unsigned long currentMillis;
};

#endif
