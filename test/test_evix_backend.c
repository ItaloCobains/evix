#include "unity.h"
#include "evix.h"
#include "test_helpers.h"
#include <time.h>

void setUp(void) {}
void tearDown(void) {}

/* -- fake backend -- */

static int fake_init_count;
static int fake_destroy_count;
static int fake_poll_count;
static int fake_io_add_count;
static int fake_io_del_count;

static int fake_init(evix_loop_t *loop)
{
    (void)loop;
    fake_init_count++;
    return 0;
}

static void fake_destroy(evix_loop_t *loop)
{
    (void)loop;
    fake_destroy_count++;
}

static int fake_poll(evix_loop_t *loop, int timeout_ms)
{
    (void)loop;
    fake_poll_count++;
    struct timespec ts;
    ts.tv_sec = (time_t)(timeout_ms / SEC_TO_MS);
    ts.tv_nsec = (long)((timeout_ms % SEC_TO_MS) * MS_TO_NS);
    nanosleep(&ts, NULL);
    return 0;
}

static int fake_io_add(evix_loop_t *loop, int fd, int events)
{
    (void)loop;
    (void)fd;
    (void)events;
    fake_io_add_count++;
    return 0;
}

static int fake_io_del(evix_loop_t *loop, int fd)
{
    (void)loop;
    (void)fd;
    fake_io_del_count++;
    return 0;
}

static evix_backend_t fake_backend = {
    .init    = fake_init,
    .destroy = fake_destroy,
    .poll    = fake_poll,
    .io_add  = fake_io_add,
    .io_del  = fake_io_del,
};

static void reset_fake_counts(void)
{
    fake_init_count = 0;
    fake_destroy_count = 0;
    fake_poll_count = 0;
    fake_io_add_count = 0;
    fake_io_del_count = 0;
}

/* -- tests -- */

void test_loop_uses_backend_init_and_destroy(void)
{
    reset_fake_counts();

    evix_loop_t *loop = evix_loop_create(&fake_backend);
    TEST_ASSERT_NOT_NULL(loop);
    TEST_ASSERT_EQUAL_INT(1, fake_init_count);
    TEST_ASSERT_EQUAL_INT(0, fake_destroy_count);

    evix_loop_destroy(loop);
    TEST_ASSERT_EQUAL_INT(1, fake_destroy_count);
}

void test_loop_uses_backend_poll_for_timers(void)
{
    reset_fake_counts();

    evix_loop_t *loop = evix_loop_create(&fake_backend);
    int value = 0;

    evix_timer_create(loop, 50, increment_counter, &value);
    evix_loop_run(loop);

    TEST_ASSERT_EQUAL_INT(1, value);
    TEST_ASSERT(fake_poll_count >= 1);

    evix_loop_destroy(loop);
}

void test_io_create_calls_backend_io_add(void)
{
    reset_fake_counts();

    evix_loop_t *loop = evix_loop_create(&fake_backend);
    evix_io_t *io = evix_io_create(loop, 42, EVIX_IO_READ, increment_counter, NULL);

    TEST_ASSERT_NOT_NULL(io);
    TEST_ASSERT_EQUAL_INT(1, fake_io_add_count);

    evix_loop_destroy(loop);
}

void test_io_stop_calls_backend_io_del(void)
{
    reset_fake_counts();

    evix_loop_t *loop = evix_loop_create(&fake_backend);
    evix_io_t *io = evix_io_create(loop, 42, EVIX_IO_READ, increment_counter, NULL);

    evix_io_stop(loop, io);
    TEST_ASSERT_EQUAL_INT(1, fake_io_del_count);

    evix_loop_destroy(loop);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_loop_uses_backend_init_and_destroy);
    RUN_TEST(test_loop_uses_backend_poll_for_timers);
    RUN_TEST(test_io_create_calls_backend_io_add);
    RUN_TEST(test_io_stop_calls_backend_io_del);
    return UNITY_END();
}
