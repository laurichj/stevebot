// test/test_mock_storage/test_mock_storage.cpp
// Simple test to verify MockStateStorage compiles and works in native context

#include <unity.h>
#include "native/mocks/MockStateStorage.h"

void test_mock_storage_initialization() {
    MockStateStorage storage;

    // Check default values
    TEST_ASSERT_EQUAL(0, storage.getLastMistTime());
    TEST_ASSERT_FALSE(storage.getHasEverMisted());
    TEST_ASSERT_TRUE(storage.getEnabled());
    TEST_ASSERT_EQUAL(0, storage.getSaveCallCount());
}

void test_mock_storage_save_and_retrieve() {
    MockStateStorage storage;

    // Save some values
    bool result = storage.save(12345, true, false);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(12345, storage.getLastMistTime());
    TEST_ASSERT_TRUE(storage.getHasEverMisted());
    TEST_ASSERT_FALSE(storage.getEnabled());
    TEST_ASSERT_EQUAL(1, storage.getSaveCallCount());
}

void test_mock_storage_multiple_saves() {
    MockStateStorage storage;

    storage.save(1000, false, true);
    storage.save(2000, true, true);
    storage.save(3000, true, false);

    // Should have latest values
    TEST_ASSERT_EQUAL(3000, storage.getLastMistTime());
    TEST_ASSERT_TRUE(storage.getHasEverMisted());
    TEST_ASSERT_FALSE(storage.getEnabled());
    TEST_ASSERT_EQUAL(3, storage.getSaveCallCount());
}

void test_mock_storage_setter_methods() {
    MockStateStorage storage;

    // Use test helper methods
    storage.setLastMistTime(9999);
    storage.setHasEverMisted(true);
    storage.setEnabled(false);

    TEST_ASSERT_EQUAL(9999, storage.getLastMistTime());
    TEST_ASSERT_TRUE(storage.getHasEverMisted());
    TEST_ASSERT_FALSE(storage.getEnabled());
}

void test_mock_storage_save_call_tracking() {
    MockStateStorage storage;

    storage.save(100, false, true);
    storage.save(200, false, true);
    TEST_ASSERT_EQUAL(2, storage.getSaveCallCount());

    storage.resetSaveCallCount();
    TEST_ASSERT_EQUAL(0, storage.getSaveCallCount());

    storage.save(300, false, true);
    TEST_ASSERT_EQUAL(1, storage.getSaveCallCount());
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_mock_storage_initialization);
    RUN_TEST(test_mock_storage_save_and_retrieve);
    RUN_TEST(test_mock_storage_multiple_saves);
    RUN_TEST(test_mock_storage_setter_methods);
    RUN_TEST(test_mock_storage_save_call_tracking);

    return UNITY_END();
}
