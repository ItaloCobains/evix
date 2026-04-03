#include "evix.h"
#include <_time.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define SEC_TO_MS 1000
#define MS_TO_NS 1000000

static uint64_t now_ms(void) {
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    return ((uint64_t)time_spec.tv_sec * SEC_TO_MS) + ((uint64_t)time_spec.tv_nsec / MS_TO_NS);
}

typedef struct evix_cb_node {
    evix_callback_fn callback;
    void *data;
    struct evix_cb_node *next;
} evix_cb_node_t;

struct evix_timer {
    uint64_t expire_at; // Absolute time in milliseconds (CLOCK_MONOTONIC)
    evix_callback_fn callback;
    void *data;
    struct evix_timer *next;
};

struct evix_loop {
    int running;
    evix_cb_node_t *head;
    evix_timer_t *timers;
};

evix_loop_t *evix_loop_create(void)
{
    evix_loop_t *loop = calloc(1, sizeof(evix_loop_t));
    return loop;
}

void evix_loop_destroy(evix_loop_t *loop)
{
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

    free(loop);
}

int evix_loop_run(evix_loop_t *loop)
{
    evix_cb_node_t *node = loop->head;
    while (node) {
        node->callback(node->data);
        node = node->next;
    }

    while(loop->timers) {
        uint64_t current = now_ms();

        evix_timer_t **prev = &loop->timers;
        evix_timer_t *timer = loop->timers;

        while (timer) {
            if (current >= timer->expire_at) {
                timer->callback(timer->data);
                *prev = timer->next;
                evix_timer_t *dead = timer;
                timer = timer->next;
                free(dead);
            } else {
                prev = &timer->next;
                timer = timer->next;
            }
        }
    }

    return 0;
}

void evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn callback, void *data)
{
    evix_cb_node_t *node = calloc(1, sizeof(evix_cb_node_t));
    
    if (!node) {
        return;
    }

    node->callback = callback;
    node->data = data;

    /* append to end of list */
    if (!loop->head) {
        loop->head = node;
    } else {
        evix_cb_node_t *tail = loop->head;
        while (tail->next) {
            tail = tail->next;
        }
        tail->next = node;
    }
}

evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn callback, void *data)
{
    evix_timer_t *timer = calloc(1, sizeof(evix_timer_t));

    if (!timer) {
        return NULL;
    }

    timer->expire_at = now_ms() + delay_ms;
    timer->callback = callback;
    timer->data = data;

    timer->next = loop->timers;
    loop->timers = timer;

    return timer;
}

void evix_timer_destroy(evix_timer_t *timer)
{
    free(timer);
}