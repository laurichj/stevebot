// src/NTPTimeProvider.h
#ifndef NTP_TIME_PROVIDER_H
#define NTP_TIME_PROVIDER_H

#include "ITimeProvider.h"
#include <Arduino.h>

class NTPTimeProvider : public ITimeProvider {
public:
    bool getTime(struct tm* timeinfo) override {
        return getLocalTime(timeinfo);
    }

    unsigned long getMillis() override {
        return millis();
    }
};

#endif
