// src/NVSStateStorage.cpp
#include "NVSStateStorage.h"

// NVS namespace and keys
const char* NVSStateStorage::NVS_NAMESPACE = "misting";
const char* NVSStateStorage::KEY_LAST_MIST_TIME = "lastMist";
const char* NVSStateStorage::KEY_HAS_EVER_MISTED = "hasEverMist";
const char* NVSStateStorage::KEY_ENABLED = "enabled";

NVSStateStorage::NVSStateStorage(LogCallback logger)
    : logger(logger) {
    log("NVS: Initialized");
}

NVSStateStorage::~NVSStateStorage() {
    preferences.end();
}

unsigned long NVSStateStorage::getLastMistTime() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {  // read-only mode
        log("NVS: Failed to open namespace for reading lastMistTime");
        return 0;
    }

    unsigned long value = preferences.getULong(KEY_LAST_MIST_TIME, 0);
    preferences.end();

    return value;
}

bool NVSStateStorage::getHasEverMisted() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {  // read-only mode
        log("NVS: Failed to open namespace for reading hasEverMisted");
        return false;
    }

    bool value = preferences.getBool(KEY_HAS_EVER_MISTED, false);
    preferences.end();

    return value;
}

bool NVSStateStorage::getEnabled() {
    if (!preferences.begin(NVS_NAMESPACE, true)) {  // read-only mode
        log("NVS: Failed to open namespace for reading enabled");
        return true;  // Default to enabled if NVS fails
    }

    bool value = preferences.getBool(KEY_ENABLED, true);
    preferences.end();

    return value;
}

bool NVSStateStorage::save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) {
    if (!preferences.begin(NVS_NAMESPACE, false)) {  // read-write mode
        log("NVS: Failed to open namespace for writing");
        return false;
    }

    bool success = true;

    // Save lastMistTime
    if (preferences.putULong(KEY_LAST_MIST_TIME, lastMistTime) == 0) {
        log("NVS: Failed to write lastMistTime");
        success = false;
    }

    // Save hasEverMisted
    if (preferences.putBool(KEY_HAS_EVER_MISTED, hasEverMisted) == 0) {
        log("NVS: Failed to write hasEverMisted");
        success = false;
    }

    // Save enabled
    if (preferences.putBool(KEY_ENABLED, enabled) == 0) {
        log("NVS: Failed to write enabled");
        success = false;
    }

    preferences.end();

    if (success) {
        log("NVS: State saved successfully");
    } else {
        log("NVS: State save failed");
    }

    return success;
}

void NVSStateStorage::log(const char* message) {
    if (logger) {
        logger(message);
    }
}
