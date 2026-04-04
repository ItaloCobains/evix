#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "evix.h"

static void increment_counter(void *data)
{
  int *val = (int *)data;
  (*val)++;
}

static void double_value(void *data)
{
  int *val = (int *)data;
  (*val) *= 2;
}

/* Callback that stops the loop -- useful for I/O watcher tests */
static void increment_and_stop(void *data)
{
  void **args = (void **)data;
  int *val = (int *)args[0];
  evix_loop_t *loop = (evix_loop_t *)args[1];
  (*val)++;
  evix_loop_stop(loop);
}

#endif
