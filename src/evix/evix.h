#ifndef EVIX_H
#define EVIX_H

typedef struct evix_loop evix_loop_t;

evix_loop_t* evix_loop_create(void);
void evix_loop_destroy(evix_loop_t* loop);
int evix_loop_run(evix_loop_t* loop);

#endif