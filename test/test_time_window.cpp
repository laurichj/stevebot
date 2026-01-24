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

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_misting_blocked_before_9am);
    return UNITY_END();
}
