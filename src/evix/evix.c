#include "evix.h"
#include <stdio.h>
#include <stdlib.h>

struct evix_loop {
    int running;
};

evix_loop_t* evix_loop_create(void) {
    evix_loop_t* loop = calloc(1, sizeof(evix_loop_t));
    return loop;
}

void evix_loop_destroy(evix_loop_t* loop){
    free(loop);
}

int evix_loop_run(evix_loop_t* loop){
    (void)loop;
    return 0;
}