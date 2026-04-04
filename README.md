# evix

A lightweight event loop library written in C with Ruby bindings via FFI.

Supports immediate callbacks, one-shot timers, and persistent/one-shot I/O watchers with an abstract backend system (kqueue on macOS/BSD, epoll planned for Linux).

The Ruby layer adds Fiber-based async I/O, giving you synchronous-looking code on top of non-blocking I/O.

## Structure

```
evix/
├── ext/evix/          # C library (event loop core + kqueue backend)
├── lib/               # Ruby gem (FFI bindings, Fiber-based I/O)
├── test/              # C unit tests (Unity)
├── examples/          # Ruby usage examples
├── cmake/             # CMake modules
├── CMakeLists.txt
├── evix.gemspec
├── Gemfile
└── Rakefile
```

## Building

Requires CMake 3.22+ and a C99 compiler.

```bash
rake compile        # build shared library
rake test_c         # build and run C tests
```

## Ruby Usage

```bash
gem install ffi
rake compile
```

### Callbacks and Timers

```ruby
require "evix"

loop = Evix::Loop.new

loop.add_callback { puts "immediate callback" }
loop.add_timer(500) { puts "fired after 500ms" }

loop.run
loop.destroy
```

### TCP Echo Server with Fibers

```ruby
require "evix"
require "socket"

loop = Evix::Loop.new
server = TCPServer.new("127.0.0.1", 3000)

loop.add_io(server.fileno, Evix::IO_READ) do
  ruby_client = server.accept_nonblock

  loop.spawn do
    client = loop.wrap(ruby_client)
    while (data = client.read)
      client.write(data)
    end
    client.close
  end
end

loop.run
```

## C API

```c
// Loop
evix_loop_t *evix_loop_create(evix_backend_t *backend);
void         evix_loop_destroy(evix_loop_t *loop);
int          evix_loop_run(evix_loop_t *loop);
void         evix_loop_stop(evix_loop_t *loop);

// Immediate callbacks
void evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn cb, void *data);

// Timers (one-shot)
evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms, evix_callback_fn cb, void *data);

// I/O watchers (persistent or one-shot)
evix_io_t *evix_io_create(evix_loop_t *loop, int fd, int events, evix_callback_fn cb, void *data);
void       evix_io_stop(evix_loop_t *loop, evix_io_t *io);

// Events: EVIX_IO_READ, EVIX_IO_WRITE, EVIX_IO_ONESHOT

// Backend (kqueue)
evix_backend_t *evix_kqueue_backend(void);
```

## License

MIT
