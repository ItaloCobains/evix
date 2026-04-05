#ifndef EVIX_INTERNAL_H
#define EVIX_INTERNAL_H

#include "evix.h"
#include <stdint.h>

typedef struct evix_cb_node {
  evix_callback_fn callback;
  void *data;
  struct evix_cb_node *next;
} evix_cb_node_t;

struct evix_timer {
  uint64_t expire_at;
  uint64_t repeat_ms;
  evix_callback_fn callback;
  void *data;
  struct evix_timer *next;
};

struct evix_io {
  int fd;
  int events;
  int oneshot;
  evix_callback_fn callback;
  void *data;
  struct evix_io *next;
};

struct evix_loop {
  int running;
  evix_cb_node_t *head;
  evix_timer_t *timers;
  evix_io_t *ios;
  evix_backend_t *backend;
  void *backend_data;
};

#endif
