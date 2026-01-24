// src/ITimeProvider.h
#ifndef I_TIME_PROVIDER_H
#define I_TIME_PROVIDER_H

#include <time.h>

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;

    // Returns true if time is available and sets timeinfo
    virtual bool getTime(struct tm* timeinfo) = 0;

    // Returns current millis() value for duration tracking
    virtual unsigned long getMillis() = 0;
};

#endif
