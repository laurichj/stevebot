// test/test_state_machine.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"

void test_initial_state_is_waiting_sync() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    // Mock time as unavailable initially
    timeProvider.setTimeAvailable(false);

    MistingScheduler scheduler(&timeProvider, &relay);

    // Don't call update - check initial state
    TEST_ASSERT_EQUAL(WAITING_SYNC, scheduler.getState());
}

void test_transitions_to_idle_when_time_available() {
    MockTimeProvider timeProvider;
    MockRelayController relay;

    timeProvider.setTimeAvailable(false);
    MistingScheduler scheduler(&timeProvider, &relay);

    TEST_ASSERT_EQUAL(WAITING_SYNC, scheduler.getState());

    // Make time available (outside active window to stay in IDLE)
    timeProvider.setTimeAvailable(true);
    timeProvider.setHour(8);  // Before 9am window
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_transitions_to_misting_when_conditions_met() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);  // In window
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_TRUE(relay.getIsOn());
}

void test_transitions_to_idle_after_25_seconds() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    scheduler.update();  // Start misting

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());

    // Advance time by 25 seconds
    timeProvider.advanceMillis(25000);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_FALSE(relay.getIsOn());
}

void test_stays_idle_when_outside_window() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(8);  // Before window
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Multiple updates shouldn't change state
    scheduler.update();
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state_is_waiting_sync);
    RUN_TEST(test_transitions_to_idle_when_time_available);
    RUN_TEST(test_transitions_to_misting_when_conditions_met);
    RUN_TEST(test_transitions_to_idle_after_25_seconds);
    RUN_TEST(test_stays_idle_when_outside_window);
    return UNITY_END();
}
