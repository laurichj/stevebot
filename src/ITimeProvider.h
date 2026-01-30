// src/ITimeProvider.h
#ifndef I_TIME_PROVIDER_H
#define I_TIME_PROVIDER_H

#include <time.h>

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;

    // Returns true if time is available and sets timeinfo
    virtual bool getTime(struct tm* timeinfo) = 0;

    // Returns current millis() value for short duration tracking (e.g., mist duration)
    virtual unsigned long getMillis() = 0;

    // Returns current Unix epoch time in seconds (0 if not available)
    virtual time_t getEpochTime() = 0;
};

#endif
