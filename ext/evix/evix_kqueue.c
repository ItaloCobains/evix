#include "evix_kqueue.h"
#include "evix_internal.h"
#include <sys/event.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
  int kq;
} evix_kqueue_state_t;

static int kqueue_init(evix_loop_t *loop)
{
  evix_kqueue_state_t *state = calloc(1, sizeof(evix_kqueue_state_t));
  if (!state) return -1;

  state->kq = kqueue();
  if (state->kq == -1) {
    free(state);
    return -1;
  }

  loop->backend_data = state;
  return 0;
}

static void kqueue_destroy(evix_loop_t *loop)
{
  evix_kqueue_state_t *state = loop->backend_data;
  if (state) {
    if (state->kq != -1)
      close(state->kq);
    free(state);
    loop->backend_data = NULL;
  }
}

static int kqueue_io_add(evix_loop_t *loop, int fd, int events)
{
  evix_kqueue_state_t *state = loop->backend_data;
  struct kevent ev;
  unsigned short flags = EV_ADD;

  if (events & EVIX_IO_ONESHOT)
    flags |= EV_ONESHOT;

  if (events & EVIX_IO_READ) {
    EV_SET(&ev, fd, EVFILT_READ, flags, 0, 0, NULL);
    if (kevent(state->kq, &ev, 1, NULL, 0, NULL) == -1)
      return -1;
  }
  if (events & EVIX_IO_WRITE) {
    EV_SET(&ev, fd, EVFILT_WRITE, flags, 0, 0, NULL);
    if (kevent(state->kq, &ev, 1, NULL, 0, NULL) == -1)
      return -1;
  }
  return 0;
}

static int kqueue_io_del(evix_loop_t *loop, int fd)
{
  evix_kqueue_state_t *state = loop->backend_data;
  struct kevent ev;

  EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  kevent(state->kq, &ev, 1, NULL, 0, NULL);

  EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  kevent(state->kq, &ev, 1, NULL, 0, NULL);

  return 0;
}

static int kqueue_poll(evix_loop_t *loop, int timeout_ms)
{
  evix_kqueue_state_t *state = loop->backend_data;
  struct kevent events[64];
  struct timespec ts;
  struct timespec *tsp = NULL;

  if (timeout_ms >= 0) {
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
    tsp = &ts;
  }

  int n = kevent(state->kq, NULL, 0, events, 64, tsp);

  for (int i = 0; i < n; i++) {
    int fd = (int)events[i].ident;
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

static evix_backend_t kqueue_backend = {
  .init    = kqueue_init,
  .destroy = kqueue_destroy,
  .poll    = kqueue_poll,
  .io_add  = kqueue_io_add,
  .io_del  = kqueue_io_del,
};

evix_backend_t *evix_kqueue_backend(void)
{
  return &kqueue_backend;
}
