# evix

> **Projeto de estudo** -- este não é um projeto de produção. Foi criado para aprender sobre event loops, I/O não-bloqueante e concorrência com Fibers, implementando tudo do zero em C com bindings para Ruby.

Event loop escrito em C com bindings nativos para Ruby (via `ruby.h`). Suporta callbacks imediatos, timers (one-shot e recorrentes), I/O watchers, sinais e concorrência baseada em Fibers.

## O que foi implementado

- **Event loop** com callbacks imediatos, timers e I/O watchers
- **Backends de I/O**: kqueue (macOS/BSD), epoll (Linux)
- **Timers**: one-shot e recorrentes, com cancel
- **I/O watchers**: persistentes e one-shot (EVIX_IO_ONESHOT)
- **Extensão C nativa para Ruby** (ruby.h, sem FFI)
- **Fibers**: concorrência cooperativa com I/O async
- **Sinais**: tratamento de sinais integrado ao loop (SIGUSR1, SIGINT, etc)
- **TCP**: server (fiber por conexão) e client non-blocking

## Estrutura

```
evix/
├── ext/evix/          # Biblioteca C (event loop + backends kqueue/epoll)
├── ext/evix_ruby/     # Extensão C nativa para Ruby
├── lib/               # Ruby (Fiber I/O, TCP server/client, sinais)
├── test/              # Testes C (Unity) e Ruby (Minitest)
├── examples/          # Exemplos de uso
├── cmake/             # Módulos CMake
└── Gemfile
```

## Build

Requer CMake 3.22+, compilador C99 e Ruby 3.x.

```bash
# Compilar extensão Ruby
cd ext/evix_ruby && ruby extconf.rb && make

# Testes C
rake test_c

# Rodar exemplos
ruby -I ext/evix_ruby -I lib examples/basic.rb
```

## Exemplos

### Callbacks e timers

```ruby
require "evix"

loop = Evix::Loop.new

loop.add_callback { puts "callback imediato" }
loop.add_timer(500) { puts "timer 500ms" }
loop.add_timer(200, repeat: 200) { puts "tick a cada 200ms" }

loop.run
loop.destroy
```

### Fibers concorrentes

```ruby
loop = Evix::Loop.new

loop.spawn do
  wrapped = loop.wrap(io)
  data = wrapped.read
  wrapped.write("resposta: #{data}")
  wrapped.close
end

loop.run
```

### Sinais

```ruby
loop = Evix::Loop.new

loop.add_signal("USR1") { puts "SIGUSR1 recebido" }
loop.add_signal("INT") { puts "Ctrl+C"; loop.stop }

loop.run
loop.destroy
```

### TCP echo server

```ruby
require "socket"

loop = Evix::Loop.new

server = Evix::TCPServer.new(loop, "127.0.0.1", 3000) do |client|
  while (data = client.read)
    client.write(data)
  end
end

loop.add_signal("INT") { server.close; loop.stop }
loop.run
loop.destroy
```

### TCP client

```ruby
loop = Evix::Loop.new

loop.spawn do
  conn = Evix::TCPClient.connect(loop, "127.0.0.1", 3000)
  conn.write("hello\n")
  puts conn.read
  conn.close
  loop.stop
end

loop.run
loop.destroy
```

## C API

```c
// Loop
evix_loop_t *evix_loop_create(evix_backend_t *backend);
void         evix_loop_destroy(evix_loop_t *loop);
int          evix_loop_run(evix_loop_t *loop);
void         evix_loop_stop(evix_loop_t *loop);

// Callbacks imediatos
void evix_loop_add_callback(evix_loop_t *loop, evix_callback_fn cb, void *data);

// Timers (one-shot ou recorrente)
evix_timer_t *evix_timer_create(evix_loop_t *loop, uint64_t delay_ms,
                                uint64_t repeat_ms, evix_callback_fn cb, void *data);
void          evix_timer_stop(evix_loop_t *loop, evix_timer_t *timer);

// I/O watchers
evix_io_t *evix_io_create(evix_loop_t *loop, int fd, int events,
                          evix_callback_fn cb, void *data);
void       evix_io_stop(evix_loop_t *loop, evix_io_t *io);

// Flags: EVIX_IO_READ, EVIX_IO_WRITE, EVIX_IO_ONESHOT

// Backends
evix_backend_t *evix_kqueue_backend(void);  // macOS/BSD
evix_backend_t *evix_epoll_backend(void);   // Linux
```

## Licença

MIT
