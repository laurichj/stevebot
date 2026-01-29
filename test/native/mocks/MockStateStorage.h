// test/native/mocks/MockStateStorage.h
#ifndef MOCK_STATE_STORAGE_H
#define MOCK_STATE_STORAGE_H

#include "IStateStorage.h"

/**
 * Mock implementation of IStateStorage for native unit tests.
 * Stores state in memory using simple member variables.
 * No dependencies on ESP32-specific libraries.
 */
class MockStateStorage : public IStateStorage {
public:
    MockStateStorage()
        : lastMistTime(0),
          hasEverMisted(false),
          enabled(true),
          saveCallCount(0) {
    }

    // IStateStorage interface implementation
    unsigned long getLastMistTime() override {
        return lastMistTime;
    }

    bool getHasEverMisted() override {
        return hasEverMisted;
    }

    bool getEnabled() override {
        return enabled;
    }

    bool save(unsigned long lastMistTime, bool hasEverMisted, bool enabled) override {
        this->lastMistTime = lastMistTime;
        this->hasEverMisted = hasEverMisted;
        this->enabled = enabled;
        saveCallCount++;
        return true;
    }

    // Test helper methods
    void setLastMistTime(unsigned long time) { lastMistTime = time; }
    void setHasEverMisted(bool value) { hasEverMisted = value; }
    void setEnabled(bool value) { enabled = value; }
    int getSaveCallCount() const { return saveCallCount; }
    void resetSaveCallCount() { saveCallCount = 0; }

private:
    unsigned long lastMistTime;
    bool hasEverMisted;
    bool enabled;
    int saveCallCount;  // Track number of times save() was called
};

#endif
