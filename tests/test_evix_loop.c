#include "unity.h"
#include "evix.h"
#include <unity_internals.h>

void setUp(void) {}
void tearDown(void) {}

void test_loop_create_and_destroy(void) {
    evix_loop_t *loop = evix_loop_create();
    TEST_ASSERT_NOT_NULL(loop);
    evix_loop_destroy(loop);
}

void test_loop_run_returns_immediately_when_empty(void) {
    evix_loop_t *loop = evix_loop_create();
    int result = evix_loop_run(loop);
    TEST_ASSERT_EQUAL_INT(0, result);
    evix_loop_destroy(loop);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_loop_create_and_destroy);
    RUN_TEST(test_loop_run_returns_immediately_when_empty);
    return UNITY_END();
}