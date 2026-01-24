// test/test_time_window.cpp
#include <unity.h>
#include "MistingScheduler.h"
#include "native/mocks/MockTimeProvider.h"
#include "native/mocks/MockRelayController.h"

void test_misting_blocked_before_9am() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(8);  // 8am - before window
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_misting_allowed_at_9am() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(9);  // 9am - start of window
    scheduler.update();

    TEST_ASSERT_TRUE(relay.getIsOn());
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_misting_allowed_at_5pm() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(17);  // 5pm - end of window
    scheduler.update();

    TEST_ASSERT_TRUE(relay.getIsOn());
    TEST_ASSERT_EQUAL(MISTING, scheduler.getState());
}

void test_misting_blocked_at_6pm() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setHour(18);  // 6pm - after window
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void test_misting_blocked_when_time_unavailable() {
    MockTimeProvider timeProvider;
    MockRelayController relay;
    MistingScheduler scheduler(&timeProvider, &relay);

    timeProvider.setTimeAvailable(false);
    scheduler.update();

    TEST_ASSERT_FALSE(relay.getIsOn());
    TEST_ASSERT_EQUAL(IDLE, scheduler.getState());
}

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_misting_blocked_before_9am);
    RUN_TEST(test_misting_allowed_at_9am);
    RUN_TEST(test_misting_allowed_at_5pm);
    RUN_TEST(test_misting_blocked_at_6pm);
    RUN_TEST(test_misting_blocked_when_time_unavailable);
    return UNITY_END();
}
