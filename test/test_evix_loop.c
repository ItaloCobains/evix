#include "unity.h"
#include "evix.h"
#include "test_helpers.h"
#include <stdint.h>
#include <time.h>

void setUp(void) {}
void tearDown(void) {}

void test_loop_create_and_destroy(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  TEST_ASSERT_NOT_NULL(loop);
  evix_loop_destroy(loop);
}

void test_loop_run_returns_immediately_when_empty(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int result = evix_loop_run(loop);
  TEST_ASSERT_EQUAL_INT(0, result);
  evix_loop_destroy(loop);
}

void test_single_callback_is_called(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 0;

  evix_loop_add_callback(loop, increment_counter, &value);
  evix_loop_run(loop);

  TEST_ASSERT_EQUAL_INT(1, value);
  evix_loop_destroy(loop);
}

void test_multiple_callbacks_called_in_order(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 1;

  evix_loop_add_callback(loop, double_value, &value);      /* 1 * 2 = 2 */
  evix_loop_add_callback(loop, increment_counter, &value); /* 2 + 1 = 3 */

  evix_loop_run(loop);

  TEST_ASSERT_EQUAL_INT(3, value);
  evix_loop_destroy(loop);
}

void test_callback_not_called_before_run(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 0;

  evix_loop_add_callback(loop, increment_counter, &value);

  TEST_ASSERT_EQUAL_INT(0, value);

  evix_loop_run(loop);
  TEST_ASSERT_EQUAL_INT(1, value);
  evix_loop_destroy(loop);
}

void test_timer_with_zero_delay(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 0;

  evix_timer_create(loop, 0, 0, increment_counter, &value);
  evix_loop_run(loop);

  TEST_ASSERT_EQUAL_INT(1, value);
  evix_loop_destroy(loop);
}

void test_timer_fires_after_delay(void)
{
  struct timespec start, end;
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 0;

  evix_timer_create(loop, 50, 0, increment_counter, &value);
  TEST_ASSERT_EQUAL_INT(0, value);

  clock_gettime(CLOCK_MONOTONIC, &start);
  evix_loop_run(loop);
  clock_gettime(CLOCK_MONOTONIC, &end);

  uint64_t elapsed_ms = (uint64_t)(end.tv_sec - start.tv_sec) * SEC_TO_MS
                      + (uint64_t)(end.tv_nsec - start.tv_nsec) / MS_TO_NS;
  TEST_ASSERT(elapsed_ms >= 50);

  TEST_ASSERT_EQUAL_INT(1, value);
  evix_loop_destroy(loop);
}

void test_recurring_timer_fires_multiple_times(void)
{
  evix_loop_t *loop = evix_loop_create(NULL);
  int value = 0;

  evix_timer_create(loop, 20, 20, increment_counter, &value);
  evix_timer_create(loop, 70, 0, stop_loop, loop);
  evix_loop_run(loop);

  TEST_ASSERT(value >= 3);
  evix_loop_destroy(loop);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_loop_create_and_destroy);
  RUN_TEST(test_loop_run_returns_immediately_when_empty);
  RUN_TEST(test_single_callback_is_called);
  RUN_TEST(test_multiple_callbacks_called_in_order);
  RUN_TEST(test_callback_not_called_before_run);
  RUN_TEST(test_timer_with_zero_delay);
  RUN_TEST(test_timer_fires_after_delay);
  RUN_TEST(test_recurring_timer_fires_multiple_times);
  return UNITY_END();
}
