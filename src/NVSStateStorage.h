// src/NVSStateStorage.h
#ifndef NVS_STATE_STORAGE_H
#define NVS_STATE_STORAGE_H

#include "IStateStorage.h"
#include <Preferences.h>

// Logging callback type (same as MistingScheduler)
typedef void (*LogCallback)(const char* message);

/**
 * ESP32 Non-Volatile Storage (NVS) implementation of IStateStorage.
 * Uses the Preferences library to store state data in flash memory
 * that persists across power cycles and reboots.
 */
class NVSStateStorage : public IStateStorage {
public:
    /**
     * Constructor
     * @param logger Optional logging callback for debugging NVS operations
     */
    NVSStateStorage(LogCallback logger = nullptr);

    /**
     * Destructor - ensures NVS is properly closed
     */
    ~NVSStateStorage();

    // IStateStorage interface implementation
    unsigned long getLastMistTime() override;
    bool getHasEverMisted() override;
    bool getEnabled() override;
    bool save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) override;

private:
    Preferences preferences;
    LogCallback logger;

    void log(const char* message);

    // NVS keys
    static const char* NVS_NAMESPACE;
    static const char* KEY_LAST_MIST_TIME;
    static const char* KEY_HAS_EVER_MISTED;
    static const char* KEY_ENABLED;
};

#endif
