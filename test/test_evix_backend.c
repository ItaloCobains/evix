#include "unity.h"
#include "evix.h"

void setUp(void) {}
void tearDown(void) {}

/* -- fake backend -- */

static int fake_init_count;
static int fake_destroy_count;

static int fake_init(evix_loop_t * loop)
{
    (void)loop;
    fake_init_count++;
    return 0;
}

static void fake_destroy(evix_loop_t * loop)
{
    (void)loop;
    fake_destroy_count++;
}

static evix_backend_t fake_backend = {
    .init = fake_init,
    .destroy = fake_destroy,
    .pool = NULL,
};

/* -- tests -- */

void test_loop_uses_backend_init_and_destroy(void)
{
    fake_init_count = 0;
    fake_destroy_count = 0;

    evix_loop_t *loop = evix_loop_create(&fake_backend);
    TEST_ASSERT_NOT_NULL(loop);
    TEST_ASSERT_EQUAL_INT(1, fake_init_count);
    TEST_ASSERT_EQUAL_INT(0, fake_destroy_count);

    evix_loop_destroy(loop);
    TEST_ASSERT_EQUAL_INT(1, fake_init_count);
    TEST_ASSERT_EQUAL_INT(1, fake_destroy_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_loop_uses_backend_init_and_destroy);
    return UNITY_END();
}