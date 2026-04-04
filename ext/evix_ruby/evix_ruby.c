#include <ruby.h>
#include "evix.h"
#include "evix_kqueue.h"

/* ================================================================
 * Evix::Loop -- TypedData wrapper around evix_loop_t
 * ================================================================ */

typedef struct {
  evix_loop_t *loop;
  VALUE callbacks;  /* Ruby Array: prevents GC of live proc objects */
} evix_rb_loop_t;

static void loop_mark(void *ptr)
{
  evix_rb_loop_t *rb_loop = ptr;
  rb_gc_mark_movable(rb_loop->callbacks);
}

static void loop_compact(void *ptr)
{
  evix_rb_loop_t *rb_loop = ptr;
  rb_loop->callbacks = rb_gc_location(rb_loop->callbacks);
}

static void loop_free(void *ptr)
{
  evix_rb_loop_t *rb_loop = ptr;
  if (rb_loop->loop) {
    evix_loop_destroy(rb_loop->loop);
    rb_loop->loop = NULL;
  }
  xfree(rb_loop);
}

static size_t loop_memsize(const void *ptr)
{
  (void)ptr;
  return sizeof(evix_rb_loop_t);
}

static const rb_data_type_t loop_type = {
  .wrap_struct_name = "Evix::Loop",
  .function = {
    .dmark   = loop_mark,
    .dfree   = loop_free,
    .dsize   = loop_memsize,
    .dcompact = loop_compact,
  },
  .flags = RUBY_TYPED_FREE_IMMEDIATELY,
};

static evix_rb_loop_t *get_loop(VALUE self)
{
  evix_rb_loop_t *rb_loop;
  TypedData_Get_Struct(self, evix_rb_loop_t, &loop_type, rb_loop);
  if (!rb_loop->loop)
    rb_raise(rb_eRuntimeError, "loop already destroyed");
  return rb_loop;
}

/* ---- allocate / initialize ---- */

static VALUE loop_alloc(VALUE klass)
{
  evix_rb_loop_t *rb_loop;
  VALUE obj = TypedData_Make_Struct(klass, evix_rb_loop_t, &loop_type, rb_loop);
  rb_loop->loop = NULL;
  rb_loop->callbacks = rb_ary_new();
  return obj;
}

static VALUE loop_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE opts;
  rb_scan_args(argc, argv, ":", &opts);

  evix_backend_t *backend = evix_kqueue_backend();

  if (!NIL_P(opts)) {
    VALUE backend_val;
    ID kw_ids[1];
    kw_ids[0] = rb_intern("backend");
    rb_get_kwargs(opts, kw_ids, 0, 1, &backend_val);

    if (backend_val != Qundef) {
      if (backend_val == ID2SYM(rb_intern("kqueue")))
        backend = evix_kqueue_backend();
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

/* ---- callback bridge ---- */

static void ruby_callback_bridge(void *data)
{
  VALUE proc = (VALUE)data;
  rb_funcall(proc, rb_intern("call"), 0);
}

/* ---- add_callback ---- */

static VALUE loop_add_callback(VALUE self)
{
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");
  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();
  rb_ary_push(rb_loop->callbacks, proc);
  evix_loop_add_callback(rb_loop->loop, ruby_callback_bridge, (void *)proc);
  return Qnil;
}

/* ---- add_timer(delay_ms) ---- */

static VALUE loop_add_timer(VALUE self, VALUE delay_ms)
{
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");
  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();
  rb_ary_push(rb_loop->callbacks, proc);
  evix_timer_create(rb_loop->loop, NUM2ULL(delay_ms), ruby_callback_bridge, (void *)proc);
  return Qnil;
}

/* ---- add_io(fd, events = IO_READ) ---- */

static VALUE loop_add_io(int argc, VALUE *argv, VALUE self)
{
  if (!rb_block_given_p())
    rb_raise(rb_eArgError, "no block given");

  VALUE fd_val, events_val;
  rb_scan_args(argc, argv, "11", &fd_val, &events_val);

  int events = NIL_P(events_val) ? EVIX_IO_READ : NUM2INT(events_val);

  evix_rb_loop_t *rb_loop = get_loop(self);
  VALUE proc = rb_block_proc();
  rb_ary_push(rb_loop->callbacks, proc);
  evix_io_create(rb_loop->loop, NUM2INT(fd_val), events, ruby_callback_bridge, (void *)proc);
  return Qnil;
}

/* ---- run / stop / destroy ---- */

static VALUE loop_run(VALUE self)
{
  evix_rb_loop_t *rb_loop = get_loop(self);
  int rc = evix_loop_run(rb_loop->loop);
  return INT2FIX(rc);
}

static VALUE loop_stop(VALUE self)
{
  evix_rb_loop_t *rb_loop = get_loop(self);
  evix_loop_stop(rb_loop->loop);
  return Qnil;
}

static VALUE loop_destroy(VALUE self)
{
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

void Init_evix_ruby(void)
{
  VALUE mEvix = rb_define_module("Evix");

  rb_define_const(mEvix, "IO_READ",    INT2FIX(EVIX_IO_READ));
  rb_define_const(mEvix, "IO_WRITE",   INT2FIX(EVIX_IO_WRITE));
  rb_define_const(mEvix, "IO_ONESHOT", INT2FIX(EVIX_IO_ONESHOT));

  VALUE cLoop = rb_define_class_under(mEvix, "Loop", rb_cObject);
  rb_define_alloc_func(cLoop, loop_alloc);
  rb_define_method(cLoop, "initialize",   loop_initialize,   -1);
  rb_define_method(cLoop, "add_callback", loop_add_callback,  0);
  rb_define_method(cLoop, "add_timer",    loop_add_timer,     1);
  rb_define_method(cLoop, "add_io",       loop_add_io,       -1);
  rb_define_method(cLoop, "run",          loop_run,           0);
  rb_define_method(cLoop, "stop",         loop_stop,          0);
  rb_define_method(cLoop, "destroy",      loop_destroy,       0);
}
