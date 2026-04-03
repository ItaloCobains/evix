#ifndef EVIX_H
#define EVIX_H

#include <stdint.h>

typedef struct evix_loop evix_loop_t;
typedef struct evix_timer evix_timer_t;
typedef void (*evix_callback_fn)(void *data);

evix_loop_t *evix_loop_create(void);
void         evix_loop_destroy(evix_loop_t *loop);
int          evix_loop_run(evix_loop_t *loop);
void         evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback, void *data);

evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn callback, void *data);
void        evix_timer_destroy(evix_timer_t *timer);

#endif