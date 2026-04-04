#include "unity.h"
#include "evix_kqueue.h"
#include "test_helpers.h"
#include <stdint.h>
#include <time.h>
#include <unistd.h>

void setUp(void) {}
void tearDown(void) {}

void test_kqueue_timer_fires_after_delay(void)
{
    evix_loop_t *loop = evix_loop_create(evix_kqueue_backend());
    TEST_ASSERT_NOT_NULL(loop);
    int value = 0;

    evix_timer_create(loop, 50, increment_counter, &value);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    evix_loop_run(loop);
    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t elapsed_ms = (uint64_t)(end.tv_sec - start.tv_sec) * SEC_TO_MS
                        + (uint64_t)(end.tv_nsec - start.tv_nsec) / MS_TO_NS;

    TEST_ASSERT(elapsed_ms >= 50);
    TEST_ASSERT_EQUAL_INT(1, value);

    evix_loop_destroy(loop);
}

void test_kqueue_io_read_callback_fires(void)
{
    int fds[2];
    TEST_ASSERT_EQUAL_INT(0, pipe(fds));
    int read_fd = fds[0];
    int write_fd = fds[1];

    evix_loop_t *loop = evix_loop_create(evix_kqueue_backend());
    TEST_ASSERT_NOT_NULL(loop);

    int value = 0;
    void *args[2] = { &value, loop };

    write(write_fd, "x", 1);

    evix_io_create(loop, read_fd, EVIX_IO_READ, increment_and_stop, args);
    evix_loop_run(loop);

    TEST_ASSERT_EQUAL_INT(1, value);

    evix_loop_destroy(loop);
    close(read_fd);
    close(write_fd);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_kqueue_timer_fires_after_delay);
    RUN_TEST(test_kqueue_io_read_callback_fires);
    return UNITY_END();
}
