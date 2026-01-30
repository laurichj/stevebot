// test/test_safety_features/test_force_mist.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"
#include "native/mocks/MockStateStorage.h"

// Helper class to capture log messages
class LogCapture {
public:
    static void reset() {
        lastMessage[0] = '\0';
    }

    static void captureLog(const char* message) {
        strncpy(lastMessage, message, sizeof(lastMessage) - 1);
        lastMessage[sizeof(lastMessage) - 1] = '\0';
    }

    static const char* getLastMessage() {
        return lastMessage;
    }

private:
    static char lastMessage[256];
};

char LogCapture::lastMessage[256] = "";

void test_forceMist_triggers_immediate_misting_when_enabled() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    LogCapture::reset();
    MistingScheduler scheduler(&timeProvider, &relay, &storage, LogCapture::captureLog);

    // Set time available but outside active window (normally wouldn't mist)
    timeProvider.setHour(20);  // 8pm - outside 9am-6pm window
    scheduler.update();

    // Should be IDLE and not misting
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());

    // Force mist
    LogCapture::reset();
    scheduler.forceMist();

    // Should now be MISTING
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_TRUE(relay.getIsOn());
    // forceMist() calls startMisting() which logs "MIST START", so that's the last message
    TEST_ASSERT_EQUAL_STRING("MIST START", LogCapture::getLastMessage());
}

void test_forceMist_blocked_when_already_misting() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    LogCapture::reset();
    MistingScheduler scheduler(&timeProvider, &relay, &storage, LogCapture::captureLog);

    // Start normal misting
    timeProvider.setHour(10);
    scheduler.update();
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());

    // Try to force mist while already misting
    LogCapture::reset();
    scheduler.forceMist();

    // Should still be misting (from original cycle)
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    // Should have logged error
    TEST_ASSERT_EQUAL_STRING("ERROR: Already misting, cannot force", LogCapture::getLastMessage());
}

void test_forceMist_blocked_when_scheduler_disabled() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    LogCapture::reset();
    MistingScheduler scheduler(&timeProvider, &relay, &storage, LogCapture::captureLog);

    // First transition to IDLE state (outside active window)
    timeProvider.setHour(8);
    scheduler.update();
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Now disable scheduler
    scheduler.setEnabled(false);

    // Try to force mist while disabled
    LogCapture::reset();
    scheduler.forceMist();

    // Should still be IDLE
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
    // Should have logged error
    TEST_ASSERT_EQUAL_STRING("ERROR: Scheduler disabled, cannot force mist", LogCapture::getLastMessage());
}

void test_forceMist_updates_lastMistEpoch() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Set initial time (both millis and epoch)
    timeProvider.setMillis(1000000);
    timeProvider.setEpochTime(1706000000);  // Some epoch time
    timeProvider.setHour(10);

    // Force mist
    scheduler.forceMist();

    // Check lastMistEpoch was updated
    TEST_ASSERT_EQUAL(1706000000, scheduler.getLastMistEpoch());

    // Advance time
    timeProvider.advanceMillis(5000);

    // Complete the mist cycle
    timeProvider.advanceMillis(25000);
    scheduler.update();

    // lastMistEpoch should still be from the force mist start
    TEST_ASSERT_EQUAL(1706000000, scheduler.getLastMistEpoch());

    // Verify it was saved to storage (saved as unsigned long)
    TEST_ASSERT_EQUAL(1706000000, storage.getLastMistTime());
    TEST_ASSERT_TRUE(storage.getHasEverMisted());
}

void setUp(void) {
    LogCapture::reset();
}

void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_forceMist_triggers_immediate_misting_when_enabled);
    RUN_TEST(test_forceMist_blocked_when_already_misting);
    RUN_TEST(test_forceMist_blocked_when_scheduler_disabled);
    RUN_TEST(test_forceMist_updates_lastMistEpoch);
    return UNITY_END();
}
