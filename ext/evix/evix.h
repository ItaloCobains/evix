#ifndef EVIX_H
#define EVIX_H

#include <stdint.h>

#define SEC_TO_MS 1000
#define MS_TO_NS 1000000

#define EVIX_IO_READ    1
#define EVIX_IO_WRITE   2
#define EVIX_IO_ONESHOT 4

/* Opaque types */
typedef struct evix_loop evix_loop_t;
typedef struct evix_timer evix_timer_t;
typedef struct evix_io evix_io_t;

/* Backend interface */
typedef struct evix_backend {
  int  (*init)(evix_loop_t *loop);
  void (*destroy)(evix_loop_t *loop);
  int  (*poll)(evix_loop_t *loop, int timeout_ms);
  int  (*io_add)(evix_loop_t *loop, int fd, int events);
  int  (*io_del)(evix_loop_t *loop, int fd);
} evix_backend_t;

/* Callback type */
typedef void (*evix_callback_fn)(void *data);

/* Loop lifecycle */
evix_loop_t *evix_loop_create(evix_backend_t *backend);
void         evix_loop_destroy(evix_loop_t *loop);
int          evix_loop_run(evix_loop_t *loop);
void         evix_loop_stop(evix_loop_t *loop);

/* Immediate callbacks */
void         evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback, void *data);

/* Timers (one-shot) */
evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn callback, void *data);
void          evix_timer_destroy(evix_timer_t *timer);

/* I/O watchers (persistent) */
evix_io_t    *evix_io_create(evix_loop_t *loop, int fd, int events, evix_callback_fn callback, void *data);
void          evix_io_stop(evix_loop_t *loop, evix_io_t *io);

#endif
