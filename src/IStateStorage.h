// src/IStateStorage.h
#ifndef I_STATE_STORAGE_H
#define I_STATE_STORAGE_H

/**
 * Interface for persistent state storage.
 * Abstracts the storage mechanism (NVS, EEPROM, in-memory, etc.)
 * allowing for testability and flexibility.
 */
class IStateStorage {
public:
    virtual ~IStateStorage() = default;

    /**
     * Get the last misting time in milliseconds.
     * @return Last misting time, or 0 if never misted or storage unavailable.
     */
    virtual unsigned long getLastMistTime() = 0;

    /**
     * Check if the system has ever performed a misting cycle.
     * @return true if at least one misting cycle has occurred, false otherwise.
     */
    virtual bool getHasEverMisted() = 0;

    /**
     * Check if the scheduler is enabled.
     * @return true if automatic misting is enabled, false if disabled.
     */
    virtual bool getEnabled() = 0;

    /**
     * Save the current state to persistent storage.
     * @param lastMistTime Time of last misting cycle in milliseconds
     * @param hasEverMisted Whether any misting cycle has occurred
     * @param enabled Whether automatic misting is enabled
     * @return true if save succeeded, false on error
     */
    virtual bool save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) = 0;
};

#endif
