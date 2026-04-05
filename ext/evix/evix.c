#include "evix.h"
#include "evix_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <sys/types.h>
#include <time.h>

static uint64_t now_ms(void) {
  struct timespec time_spec;
  clock_gettime(CLOCK_MONOTONIC, &time_spec);
  return ((uint64_t)time_spec.tv_sec * SEC_TO_MS) +
         ((uint64_t)time_spec.tv_nsec / MS_TO_NS);
}

static void sleep_ms(uint64_t milliseconds) {
  struct timespec time_spec;
  time_spec.tv_sec = (time_t)(milliseconds / SEC_TO_MS);
  time_spec.tv_nsec = (long)((milliseconds % SEC_TO_MS) * MS_TO_NS);
  nanosleep(&time_spec, NULL);
}

/* ================================================================
 * LIFECYCLE
 * ================================================================ */

evix_loop_t *evix_loop_create(evix_backend_t *backend) {
  evix_loop_t *loop = calloc(1, sizeof(evix_loop_t));
  if (!loop)
    return NULL;

  loop->backend = backend;

  if (backend && backend->init) {
    if (backend->init(loop) != 0) {
      free(loop);
      return NULL;
    }
  }

  return loop;
}

void evix_loop_destroy(evix_loop_t *loop) {
  evix_cb_node_t *node = loop->head;
  while (node) {
    evix_cb_node_t *next = node->next;
    free(node);
    node = next;
  }

  evix_timer_t *timer = loop->timers;
  while (timer) {
    evix_timer_t *next = timer->next;
    free(timer);
    timer = next;
  }

  evix_io_t *io = loop->ios;
  while (io) {
    evix_io_t *next = io->next;
    free(io);
    io = next;
  }

  if (loop->backend && loop->backend->destroy) {
    loop->backend->destroy(loop);
  }

  free(loop);
}

void evix_loop_stop(evix_loop_t *loop) { loop->running = 0; }

/* ================================================================
 * CORE: event loop
 * ================================================================ */

static void drain_callbacks(evix_loop_t *loop) {
  while (loop->head) {
    evix_cb_node_t *node = loop->head;
    loop->head = node->next;
    node->callback(node->data);
    free(node);
  }
}

static int has_work(evix_loop_t *loop) {
  return loop->head || loop->timers || loop->ios;
}

int evix_loop_run(evix_loop_t *loop) {
  loop->running = 1;
  while (loop->running && has_work(loop)) {
    drain_callbacks(loop);

    if (!loop->running || !has_work(loop))
      break;

    /* Calculate timeout from nearest timer */
    int timeout_ms = -1;
    if (loop->timers) {
      uint64_t earliest = loop->timers->expire_at;
      evix_timer_t *t = loop->timers->next;
      while (t) {
        if (t->expire_at < earliest)
          earliest = t->expire_at;
        t = t->next;
      }
      uint64_t current = now_ms();
      if (earliest > current)
        timeout_ms = (int)(earliest - current);
      else
        timeout_ms = 0;
    }

    /* Poll for I/O and timer events */
    if (loop->backend && loop->backend->poll) {
      loop->backend->poll(loop, timeout_ms);
    } else if (timeout_ms > 0) {
      sleep_ms((uint64_t)timeout_ms);
    }

    /* Fire expired timers */
    if (loop->timers) {
      uint64_t current = now_ms();
      evix_timer_t **prev = &loop->timers;
      evix_timer_t *timer = loop->timers;
      while (timer) {
        if (current >= timer->expire_at) {
          timer->callback(timer->data);

          if (timer->repeat_ms > 0) {
            /* Re-schedule */
            timer->expire_at = current + timer->repeat_ms;
            prev = &timer->next;
            timer = timer->next;
          } else {
            /* One-shot remove */
            *prev = timer->next;
            evix_timer_t *dead = timer;
            timer = timer->next;
            free(dead);
          }
        } else {
          prev = &timer->next;
          timer = timer->next;
        }
      }
    }
  }

  return 0;
}

/* ================================================================
 * IMMEDIATE CALLBACKS
 * ================================================================ */

void evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback,
                            void *data) {
  evix_cb_node_t *node = calloc(1, sizeof(evix_cb_node_t));
  if (!node)
    return;

  node->callback = callback;
  node->data = data;

  if (!loop->head) {
    loop->head = node;
  } else {
    evix_cb_node_t *tail = loop->head;
    while (tail->next)
      tail = tail->next;
    tail->next = node;
  }
}

/* ================================================================
 * TIMERS
 * ================================================================ */

evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms,
                                uint64_t repeat_ms, evix_callback_fn callback,
                                void *data) {
  evix_timer_t *timer = calloc(1, sizeof(evix_timer_t));
  if (!timer)
    return NULL;

  timer->expire_at = now_ms() + delay_ms;
  timer->repeat_ms = repeat_ms;
  timer->callback = callback;
  timer->data = data;

  timer->next = loop->timers;
  loop->timers = timer;

  return timer;
}

void evix_timer_stop(evix_loop_t *loop, evix_timer_t *timer) {
  evix_timer_t **prev = &loop->timers;
  evix_timer_t *cur = loop->timers;
  while (cur) {
    if (cur == timer) {
      *prev = cur->next;
      free(cur);
      return;
    }
    prev = &cur->next;
    cur = cur->next;
  }
}

void evix_timer_destroy(evix_timer_t *timer) { free(timer); }

/* ================================================================
 * I/O WATCHERS
 * ================================================================ */

evix_io_t *evix_io_create(evix_loop_t *loop, int fd, int events,
                          evix_callback_fn callback, void *data) {
  if (!loop->backend || !loop->backend->io_add)
    return NULL;

  evix_io_t *io = calloc(1, sizeof(evix_io_t));
  if (!io)
    return NULL;

  io->fd = fd;
  io->events = events;
  io->oneshot = (events & EVIX_IO_ONESHOT) ? 1 : 0;
  io->callback = callback;
  io->data = data;

  io->next = loop->ios;
  loop->ios = io;

  loop->backend->io_add(loop, fd, events);

  return io;
}

void evix_io_stop(evix_loop_t *loop, evix_io_t *io) {
  if (loop->backend && loop->backend->io_del) {
    loop->backend->io_del(loop, io->fd);
  }

  evix_io_t **prev = &loop->ios;
  evix_io_t *cur = loop->ios;
  while (cur) {
    if (cur == io) {
      *prev = cur->next;
      free(cur);
      return;
    }
    prev = &cur->next;
    cur = cur->next;
  }
}
