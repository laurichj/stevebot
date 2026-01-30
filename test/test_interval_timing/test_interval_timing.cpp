// test/test_interval_timing.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"

void test_first_mist_triggers_immediately() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);  // In window
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());
}

void test_second_mist_waits_2_hours() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    timeProvider.setEpochTime(1706000000);
    scheduler.update();  // First mist starts

    // Complete first mist
    timeProvider.advanceMillis(25000);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());

    // Try to mist before 2 hours (advance epoch time by 1 hour)
    timeProvider.advanceEpochTime(3600);  // 1 hour in seconds
    timeProvider.advanceMillis(3600000);  // 1 hour in millis
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());  // Still only 1
}

void test_mist_triggers_at_2_hour_mark() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    timeProvider.setEpochTime(1706000000);
    scheduler.update();  // First mist

    // Complete first mist
    timeProvider.advanceMillis(25000);
    scheduler.update();

    // Advance exactly 2 hours from first mist start
    timeProvider.setMillis(7200000);  // 2 hours in millis
    timeProvider.setEpochTime(1706000000 + 7200);  // 2 hours in epoch seconds
    scheduler.update();

    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
    TEST_ASSERT_EQUAL(2, relay.getTurnOnCount());
}

void test_multiple_cycles_maintain_spacing() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    timeProvider.setEpochTime(1706000000);

    // First cycle
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());

    // Second cycle (2 hours later)
    timeProvider.setMillis(7200000);
    timeProvider.setEpochTime(1706000000 + 7200);
    timeProvider.setHour(12);
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(2, relay.getTurnOnCount());

    // Third cycle (4 hours from start)
    timeProvider.setMillis(14400000);
    timeProvider.setEpochTime(1706000000 + 14400);
    timeProvider.setHour(14);
    scheduler.update();
    timeProvider.advanceMillis(25000);
    scheduler.update();
    TEST_ASSERT_EQUAL(3, relay.getTurnOnCount());
}

void test_mist_blocked_before_interval_even_in_window() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(10);
    timeProvider.setEpochTime(1706000000);
    scheduler.update();  // First mist

    timeProvider.advanceMillis(25000);
    scheduler.update();  // Complete first mist

    // Still in window but only 30 minutes later
    timeProvider.setMillis(25000 + 1800000);  // +30 min in millis
    timeProvider.setEpochTime(1706000000 + 1800);  // +30 min in epoch seconds
    timeProvider.setHour(10);
    scheduler.update();

    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
    TEST_ASSERT_EQUAL(1, relay.getTurnOnCount());
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_first_mist_triggers_immediately);
    RUN_TEST(test_second_mist_waits_2_hours);
    RUN_TEST(test_mist_triggers_at_2_hour_mark);
    RUN_TEST(test_multiple_cycles_maintain_spacing);
    RUN_TEST(test_mist_blocked_before_interval_even_in_window);
    return UNITY_END();
}
