// test/test_safety_features/test_state_persistence.cpp
// Tests for state persistence functionality - verifying that state is saved
// when scheduler operations occur (startMisting, stopMisting, setEnabled)

#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"
#include "native/mocks/MockStateStorage.h"

void test_state_saved_after_start_misting() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Set up conditions for misting (in active window, first mist)
    timeProvider.setHour(10);  // 10am - in window
    timeProvider.setMillis(1000);  // Set non-zero time

    // Reset save call count to ignore any saves during construction
    storage.resetSaveCallCount();

    // Trigger misting
    scheduler.update();

    // Verify state was saved
    TEST_ASSERT_EQUAL(1, storage.getSaveCallCount());
    TEST_ASSERT_TRUE(storage.getHasEverMisted());
    TEST_ASSERT_EQUAL(1000, storage.getLastMistTime());
}

void test_state_saved_after_stop_misting() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Start misting
    timeProvider.setHour(10);
    scheduler.update();

    // Reset save call count (we already tested startMisting saves)
    storage.resetSaveCallCount();

    // Advance time to end misting (25 seconds)
    timeProvider.advanceMillis(25000);
    scheduler.update();

    // Verify state was saved after stopping
    TEST_ASSERT_GREATER_THAN(0, storage.getSaveCallCount());
}

void test_state_saved_after_set_enabled() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Reset save call count
    storage.resetSaveCallCount();

    // Disable scheduler
    scheduler.setEnabled(false);

    // Verify state was saved
    TEST_ASSERT_EQUAL(1, storage.getSaveCallCount());
    TEST_ASSERT_FALSE(storage.getEnabled());

    // Enable scheduler
    scheduler.setEnabled(true);

    // Verify state was saved again
    TEST_ASSERT_EQUAL(2, storage.getSaveCallCount());
    TEST_ASSERT_TRUE(storage.getEnabled());
}

void test_no_save_when_storage_is_nullptr() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    // Create scheduler without storage
    MistingScheduler scheduler(&timeProvider, &relay, nullptr);

    // These operations should not crash even without storage
    timeProvider.setHour(10);
    scheduler.update();  // Start misting

    timeProvider.advanceMillis(25000);
    scheduler.update();  // Stop misting

    scheduler.setEnabled(false);
    scheduler.setEnabled(true);

    // No assertion needed - just verify no crash
    TEST_ASSERT_TRUE(true);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_state_saved_after_start_misting);
    RUN_TEST(test_state_saved_after_stop_misting);
    RUN_TEST(test_state_saved_after_set_enabled);
    RUN_TEST(test_no_save_when_storage_is_nullptr);
    return UNITY_END();
}
