// test/test_safety_features/test_state_recovery.cpp
// Tests for state recovery functionality - verifying that loadState()
// correctly restores saved state from storage

#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"
#include "native/mocks/MockStateStorage.h"

void test_load_state_restores_last_mist_time() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    // Pre-populate storage with state
    storage.setLastMistTime(123456);
    storage.setHasEverMisted(true);
    storage.setEnabled(true);

    MistingScheduler scheduler(&timeProvider, &relay, &storage);

    // Load state from storage
    scheduler.loadState();

    // Verify lastMistTime was restored
    TEST_ASSERT_EQUAL(123456, scheduler.getLastMistTime());
}

void test_load_state_restores_has_ever_misted() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    // Pre-populate storage with hasEverMisted = true
    storage.setLastMistTime(100000);
    storage.setHasEverMisted(true);
    storage.setEnabled(true);

    MistingScheduler scheduler(&timeProvider, &relay, &storage);
    scheduler.loadState();

    // Verify scheduler doesn't immediately mist (because hasEverMisted is true)
    // Set time in active window
    timeProvider.setHour(10);
    scheduler.update();

    // Should remain IDLE because we haven't waited MIST_INTERVAL yet
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_load_state_restores_scheduler_enabled() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    // Pre-populate storage with schedulerEnabled = false
    storage.setLastMistTime(0);
    storage.setHasEverMisted(false);
    storage.setEnabled(false);

    MistingScheduler scheduler(&timeProvider, &relay, &storage);
    scheduler.loadState();

    // Verify scheduler is disabled
    TEST_ASSERT_FALSE(scheduler.isEnabled());

    // Verify scheduler doesn't mist even in active window
    // When disabled, scheduler stays in WAITING_SYNC and doesn't transition
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(WAITING_SYNC, scheduler.getState());

    // Re-enable and verify it transitions and starts first mist
    // (because hasEverMisted=false and we're in active window)
    scheduler.setEnabled(true);
    scheduler.update();
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_load_state_handles_nullptr_storage() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    // Create scheduler without storage
    MistingScheduler scheduler(&timeProvider, &relay, nullptr);

    // Load state should not crash
    scheduler.loadState();

    // Scheduler should use default values (enabled=true, hasEverMisted=false)
    TEST_ASSERT_TRUE(scheduler.isEnabled());

    // Should mist immediately when in active window (first mist)
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_load_state_with_zero_values_uses_defaults() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    // Storage has default (zero) values
    storage.setLastMistTime(0);
    storage.setHasEverMisted(false);
    storage.setEnabled(true);

    MistingScheduler scheduler(&timeProvider, &relay, &storage);
    scheduler.loadState();

    // Should behave like first boot - mist immediately in active window
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_load_state_prevents_immediate_remist_after_recovery() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MockStateStorage storage;

    // Simulate recovery after a recent mist (1 hour ago)
    // Using millis = 3600000 (1 hour in ms)
    unsigned long oneHourAgo = 3600000;

    storage.setLastMistTime(oneHourAgo);
    storage.setHasEverMisted(true);
    storage.setEnabled(true);

    // Set current time to 1 hour after last mist
    timeProvider.setMillis(oneHourAgo);

    MistingScheduler scheduler(&timeProvider, &relay, &storage);
    scheduler.loadState();

    // Set time in active window
    timeProvider.setHour(10);
    scheduler.update();

    // Should NOT mist because only 1 hour has passed (need 2 hours)
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Advance another hour (total 2 hours)
    timeProvider.advanceMillis(3600000);
    scheduler.update();

    // Now should start misting
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_load_state_restores_last_mist_time);
    RUN_TEST(test_load_state_restores_has_ever_misted);
    RUN_TEST(test_load_state_restores_scheduler_enabled);
    RUN_TEST(test_load_state_handles_nullptr_storage);
    RUN_TEST(test_load_state_with_zero_values_uses_defaults);
    RUN_TEST(test_load_state_prevents_immediate_remist_after_recovery);
    return UNITY_END();
}
