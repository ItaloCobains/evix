#include <ruby.h>
#include "evix.h"
#include "evix_internal.h"

#ifdef __linux__
#include "evix_epoll.h"
#define EVIX_DEFAULT_BACKEND evix_epoll_backend
#define EVIX_DEFAULT_BACKEND_SYM "epoll"
#else
#include "evix_kqueue.h"
#define EVIX_DEFAULT_BACKEND evix_kqueue_backend
#define EVIX_DEFAULT_BACKEND_SYM "kqueue"
#endif

static VALUE cTimer;
static VALUE cIOWatcher;

/* ================================================================
 * Evix::Loop -- TypedData wrapper around evix_loop_t
 * ================================================================ */

typedef struct {
  evix_loop_t *loop;
  VALUE callbacks; /* Ruby Array: prevents GC of live proc objects */
} evix_rb_loop_t;

static void loop_mark(void *ptr) {
  evix_rb_loop_t *rb_loop = ptr;
  rb_gc_mark(rb_loop->callbacks);
}

static void loop_free(void *ptr) {
  evix_rb_loop_t *rb_loop = ptr;
  if (rb_loop->loop) {
    evix_loop_destroy(rb_loop->loop);
    rb_loop->loop = NULL;
  }
  xfree(rb_loop);
}

static size_t loop_memsize(const void *ptr) {
  (void)ptr;
  return sizeof(evix_rb_loop_t);
}

static const rb_data_type_t loop_type = {
  .wrap_struct_name = "Evix::Loop",
  .function = {
    .dmark = loop_mark,
    .dfree = loop_free,
    .dsize = loop_memsize,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static evix_rb_loop_t *get_loop(VALUE self) {
  evix_rb_loop_t *rb_loop;
  TypedData_Get_Struct(self, evix_rb_loop_t, &loop_type, rb_loop);
  if (!rb_loop->loop)
    rb_raise(rb_eRuntimeError, "loop already destroyed");
  return rb_loop;
}

/* ---- allocate / initialize ---- */

static VALUE loop_alloc(VALUE klass) {
  evix_rb_loop_t *rb_loop;
  VALUE obj = TypedData_Make_Struct(klass, evix_rb_loop_t, &loop_type, rb_loop);
  rb_loop->loop = NULL;
  rb_loop->callbacks = rb_ary_new();
  return obj;
}

static VALUE loop_initialize(int argc, VALUE *argv, VALUE self) {
  VALUE opts;
  rb_scan_args(argc, argv, ":", &opts);

  evix_backend_t *backend = EVIX_DEFAULT_BACKEND();

  if (!NIL_P(opts)) {
    VALUE backend_val;
    ID kw_ids[1];
    kw_ids[0] = rb_intern("backend");
    rb_get_kwargs(opts, kw_ids, 0, 1, &backend_val);

    if (backend_val != Qundef) {
      if (backend_val == ID2SYM(rb_intern(EVIX_DEFAULT_BACKEND_SYM)))
        backend = EVIX_DEFAULT_BACKEND();
      else if (backend_val == Qnil)
        backend = NULL;
      else
        rb_raise(rb_eArgError, "unknown backend");
    }
  }

  evix_rb_loop_t *rb_loop;
  TypedData_Get_Struct(self, evix_rb_loop_t, &loop_type, rb_loop);

  rb_loop->loop = evix_loop_create(backend);
  if (!rb_loop->loop)
    rb_raise(rb_eRuntimeError, "failed to create evix loop");

  return self;
}

/* ================================================================
 * Evix::Timer -- handle returned by add_timer
 * ================================================================ */

typedef struct {
  evix_loop_t *loop;
  evix_timer_t *timer;
  VALUE proc;
} evix_rb_timer_t;

static void timer_mark(void *ptr) {
  evix_rb_timer_t *t = ptr;
  rb_gc_mark(t->proc);
}

static void timer_free(void *ptr) {
  xfree(ptr);
}

static size_t timer_memsize(const void *ptr) {
  (void)ptr;
  return sizeof(evix_rb_timer_t);
}

static const rb_data_type_t timer_type = {
  .wrap_struct_name = "Evix::Timer",
  .function = {
    .dmark = timer_mark,
    .dfree = timer_free,
    .dsize = timer_memsize,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void timer_callback_bridge(void *data) {
  VALUE timer_obj = (VALUE)data;
  evix_rb_timer_t *t;
  TypedData_Get_Struct(timer_obj, evix_rb_timer_t, &timer_type, t);

  rb_funcall(t->proc, rb_intern("call"), 0);

  /* One-shot: C frees the timer after this callback returns */
  if (t->timer && t->timer->repeat_ms == 0)
    t->timer = NULL;
}

static VALUE timer_cancel(VALUE self) {
  evix_rb_timer_t *t;
  TypedData_Get_Struct(self, evix_rb_timer_t, &timer_type, t);
  if (t->timer) {
    evix_timer_stop(t->loop, t->timer);
    t->timer = NULL;
  }
  return Qnil;
}

static VALUE timer_cancelled_p(VALUE self) {
  evix_rb_timer_t *t;
  TypedData_Get_Struct(self, evix_rb_timer_t, &timer_type, t);
  return t->timer ? Qfalse : Qtrue;
}

/* ================================================================
 * Evix::IOWatcher -- handle returned by add_io
 * ================================================================ */

typedef struct {
  evix_loop_t *loop;
  evix_io_t *io;
  VALUE proc;
} evix_rb_io_watcher_t;

static void io_watcher_mark(void *ptr) {
  evix_rb_io_watcher_t *w = ptr;
  rb_gc_mark(w->proc);
}

static void io_watcher_free(void *ptr) {
  xfree(ptr);
}

static size_t io_watcher_memsize(const void *ptr) {
  (void)ptr;
  return sizeof(evix_rb_io_watcher_t);
}

static const rb_data_type_t io_watcher_type = {
  .wrap_struct_name = "Evix::IOWatcher",
  .function = {
    .dmark = io_watcher_mark,
    .dfree = io_watcher_free,
    .dsize = io_watcher_memsize,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static void io_callback_bridge(void *data) {
  VALUE watcher_obj = (VALUE)data;
  evix_rb_io_watcher_t *w;
  TypedData_Get_Struct(watcher_obj, evix_rb_io_watcher_t, &io_watcher_type, w);

  rb_funcall(w->proc, rb_intern("call"), 0);

  /* Oneshot: C frees the io watcher after this callback returns */
  if (w->io && w->io->oneshot)
    w->io = NULL;
}

static VALUE io_watcher_cancel(VALUE self) {
  evix_rb_io_watcher_t *w;
  TypedData_Get_Struct(self, evix_rb_io_watcher_t, &io_watcher_type, w);
  if (w->io) {
    evix_io_stop(w->loop, w->io);
    w->io = NULL;
  }
  return Qnil;
}

static VALUE io_watcher_cancelled_p(VALUE self) {
  evix_rb_io_watcher_t *w;
  TypedData_Get_Struct(self, evix_rb_io_watcher_t, &io_watcher_type, w);
  return w->io ? Qfalse : Qtrue;
}

/* ================================================================
 * Loop methods
 * ================================================================ */

/* ---- callback bridge (for add_callback only) ---- */

static void ruby_callback_bridge(void *data) {
  VALUE proc = (VALUE)data;
  rb_funcall(proc, rb_intern("call"), 0);
}

/* ---- add_callback ---- */

static VALUE loop_add_callback(VALUE self) {
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");
  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();
  rb_ary_push(rb_loop->callbacks, proc);
  evix_loop_add_callback(rb_loop->loop, ruby_callback_bridge, (void *)proc);
  return Qnil;
}

/* ---- add_timer(delay_ms, repeat: nil) -> Timer ---- */

static VALUE loop_add_timer(int argc, VALUE *argv, VALUE self) {
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");

  VALUE delay_ms, opts;
  rb_scan_args(argc, argv, "1:", &delay_ms, &opts);

  uint64_t repeat = 0;
  if (!NIL_P(opts)) {
    VALUE repeat_val;
    ID kw_ids[1];
    kw_ids[0] = rb_intern("repeat");
    rb_get_kwargs(opts, kw_ids, 0, 1, &repeat_val);
    if (repeat_val != Qundef && !NIL_P(repeat_val))
      repeat = NUM2ULL(repeat_val);
  }

  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();

  evix_rb_timer_t *rb_timer;
  VALUE timer_obj = TypedData_Make_Struct(cTimer, evix_rb_timer_t, &timer_type, rb_timer);
  rb_timer->loop = rb_loop->loop;
  rb_timer->proc = proc;

  rb_ary_push(rb_loop->callbacks, timer_obj);

  rb_timer->timer = evix_timer_create(rb_loop->loop, NUM2ULL(delay_ms), repeat,
                                      timer_callback_bridge, (void *)timer_obj);

  return timer_obj;
}

/* ---- add_io(fd, events = IO_READ) -> IOWatcher ---- */

static VALUE loop_add_io(int argc, VALUE *argv, VALUE self) {
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");

  VALUE fd_val, events_val;
  rb_scan_args(argc, argv, "11", &fd_val, &events_val);

  int events = NIL_P(events_val) ? EVIX_IO_READ : NUM2INT(events_val);

  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();

  evix_rb_io_watcher_t *rb_watcher;
  VALUE watcher_obj = TypedData_Make_Struct(cIOWatcher, evix_rb_io_watcher_t,
                                           &io_watcher_type, rb_watcher);
  rb_watcher->loop = rb_loop->loop;
  rb_watcher->proc = proc;

  rb_ary_push(rb_loop->callbacks, watcher_obj);

  rb_watcher->io = evix_io_create(rb_loop->loop, NUM2INT(fd_val), events,
                                  io_callback_bridge, (void *)watcher_obj);

  return watcher_obj;
}

/* ---- run / stop / destroy ---- */

static VALUE loop_run(VALUE self) {
  evix_rb_loop_t *rb_loop = get_loop(self);
  int rc = evix_loop_run(rb_loop->loop);
  return INT2FIX(rc);
}

static VALUE loop_stop(VALUE self) {
  evix_rb_loop_t *rb_loop = get_loop(self);
  evix_loop_stop(rb_loop->loop);
  return Qnil;
}

static VALUE loop_destroy(VALUE self) {
  evix_rb_loop_t *rb_loop;
  TypedData_Get_Struct(self, evix_rb_loop_t, &loop_type, rb_loop);
  if (rb_loop->loop) {
    evix_loop_destroy(rb_loop->loop);
    rb_loop->loop = NULL;
    rb_ary_clear(rb_loop->callbacks);
  }
  return Qnil;
}

/* ================================================================
 * Init
 * ================================================================ */

void Init_evix_ruby(void) {
  VALUE mEvix = rb_define_module("Evix");

  rb_define_const(mEvix, "IO_READ", INT2FIX(EVIX_IO_READ));
  rb_define_const(mEvix, "IO_WRITE", INT2FIX(EVIX_IO_WRITE));
  rb_define_const(mEvix, "IO_ONESHOT", INT2FIX(EVIX_IO_ONESHOT));

  /* Evix::Loop */
  VALUE cLoop = rb_define_class_under(mEvix, "Loop", rb_cObject);
  rb_define_alloc_func(cLoop, loop_alloc);
  rb_define_method(cLoop, "initialize", loop_initialize, -1);
  rb_define_method(cLoop, "add_callback", loop_add_callback, 0);
  rb_define_method(cLoop, "add_timer", loop_add_timer, -1);
  rb_define_method(cLoop, "add_io", loop_add_io, -1);
  rb_define_method(cLoop, "run", loop_run, 0);
  rb_define_method(cLoop, "stop", loop_stop, 0);
  rb_define_method(cLoop, "destroy", loop_destroy, 0);

  /* Evix::Timer */
  cTimer = rb_define_class_under(mEvix, "Timer", rb_cObject);
  rb_undef_alloc_func(cTimer);
  rb_define_method(cTimer, "cancel", timer_cancel, 0);
  rb_define_method(cTimer, "cancelled?", timer_cancelled_p, 0);

  /* Evix::IOWatcher */
  cIOWatcher = rb_define_class_under(mEvix, "IOWatcher", rb_cObject);
  rb_undef_alloc_func(cIOWatcher);
  rb_define_method(cIOWatcher, "cancel", io_watcher_cancel, 0);
  rb_define_method(cIOWatcher, "cancelled?", io_watcher_cancelled_p, 0);
}
