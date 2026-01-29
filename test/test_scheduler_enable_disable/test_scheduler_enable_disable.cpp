// test/test_safety_features/test_scheduler_enable_disable.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"
#include "native/mocks/MockStateStorage.h"

void test_setEnabled_false_disables_automatic_misting() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Set to active window, should normally trigger misting
    timeProvider.setHour(10);

    // First, let scheduler transition to IDLE (outside window) or ready to mist
    // Use hour outside window so it transitions to IDLE without misting
    timeProvider.setHour(8);  // Before active window
    scheduler.update();
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Now disable scheduler and set back to active window
    scheduler.setEnabled(false);
    timeProvider.setHour(10);

    // Update - should NOT start misting because disabled
    scheduler.update();

    // Should still be IDLE (not MISTING) because scheduler is disabled
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
}

void test_setEnabled_true_enables_automatic_misting() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Disable first
    scheduler.setEnabled(false);
    TEST_ASSERT_FALSE(scheduler.isEnabled());

    // Re-enable
    scheduler.setEnabled(true);
    TEST_ASSERT_TRUE(scheduler.isEnabled());

    // Set to active window and update - should start misting
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_TRUE(relay.getIsOn());
}

void test_disabled_scheduler_prevents_scheduled_mists() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Start with enabled, trigger first mist
    timeProvider.setHour(10);
    scheduler.update();
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());

    // Complete the mist cycle
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Disable scheduler
    scheduler.setEnabled(false);

    // Advance time by 2 hours (normal interval)
    timeProvider.advanceMillis(7200000);

    // Update - should NOT start misting even though interval passed
    scheduler.update();
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
}

void test_enabled_flag_is_saved_to_storage() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Reset save counter
    storage.resetSaveCallCount();
    int initialSaveCount = storage.getSaveCallCount();

    // Disable scheduler - should trigger save
    scheduler.setEnabled(false);

    TEST_ASSERT_EQUAL(initialSaveCount + 1, storage.getSaveCallCount());
    TEST_ASSERT_FALSE(storage.getEnabled());

    // Re-enable - should trigger another save
    scheduler.setEnabled(true);

    TEST_ASSERT_EQUAL(initialSaveCount + 2, storage.getSaveCallCount());
    TEST_ASSERT_TRUE(storage.getEnabled());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_setEnabled_false_disables_automatic_misting);
    RUN_TEST(test_setEnabled_true_enables_automatic_misting);
    RUN_TEST(test_disabled_scheduler_prevents_scheduled_mists);
    RUN_TEST(test_enabled_flag_is_saved_to_storage);
    return UNITY_END();
}
