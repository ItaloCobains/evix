#ifdef __linux__

#include "evix_epoll.h"
#include "evix_internal.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
  int epfd;
} evix_epoll_state_t;

static int epoll_ev_init(evix_loop_t *loop) {
  evix_epoll_state_t *state = calloc(1, sizeof(evix_epoll_state_t));
  if (!state) return -1;

  state->epfd = epoll_create1(0);
  if (state->epfd == -1) {
    free(state);
    return -1;
  }

  loop->backend_data = state;
  return 0;
}

static void epoll_ev_destroy(evix_loop_t *loop) {
  evix_epoll_state_t *state = loop->backend_data;
  if (state) {
    if (state->epfd != -1)
      close(state->epfd);
    free(state);
    loop->backend_data = NULL;
  }
}

static int epoll_io_add(evix_loop_t *loop, int fd, int events) {
  evix_epoll_state_t *state = loop->backend_data;
  struct epoll_event ev = {0};
  ev.data.fd = fd;

  if (events & EVIX_IO_READ) ev.events |= EPOLLIN;
  if (events & EVIX_IO_WRITE) ev.events |= EPOLLOUT;
  if (events & EVIX_IO_ONESHOT) ev.events |= EPOLLONESHOT;

  return epoll_ctl(state->epfd, EPOLL_CTL_ADD, fd, &ev);
}

static int epoll_io_del(evix_loop_t *loop, int fd) {
  evix_epoll_state_t *state = loop->backend_data;
  return epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, NULL);
}

static int epoll_poll(evix_loop_t *loop, int timeout_ms) {
  evix_epoll_state_t *state = loop->backend_data;
  struct epoll_event events[64];

  int n = epoll_wait(state->epfd, events, 64, timeout_ms);

  for (int i = 0; i < n; i++) {
    int fd = events[i].data.fd;
    evix_io_t **prev = &loop->ios;
    evix_io_t *io = loop->ios;
    while (io) {
      if (io->fd == fd) {
        io->callback(io->data);
        if (io->oneshot) {
          *prev = io->next;
          free(io);
        }
        break;
      }
      prev = &io->next;
      io = io->next;
    }
  }

  return n;
}

static evix_backend_t epoll_backend = {
  .init    = epoll_ev_init,
  .destroy = epoll_ev_destroy,
  .poll    = epoll_poll,
  .io_add  = epoll_io_add,
  .io_del  = epoll_io_del,
};

evix_backend_t *evix_epoll_backend(void) {
  return &epoll_backend;
}

#endif
