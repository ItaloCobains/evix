# frozen_string_literal: true

# Demonstra I/O watchers one-shot vs persistentes.

require_relative '../lib/evix'

loop = Evix::Loop.new

rd1, wr1 = IO.pipe
rd2, wr2 = IO.pipe

# Watcher ONE-SHOT: dispara uma vez e se remove
watcher = loop.add_io(rd1.fileno, Evix::IO_READ | Evix::IO_ONESHOT) do
  msg = rd1.read_nonblock(1024)
  puts "[oneshot] leu: #{msg}"
  puts "[oneshot] disparou (oneshot se auto-remove)"
end

# Watcher PERSISTENTE: continua ativo
persistent_count = 0
persistent = loop.add_io(rd2.fileno, Evix::IO_READ) do
  msg = rd2.read_nonblock(1024)
  persistent_count += 1
  puts "[persistente ##{persistent_count}] leu: #{msg}"
end

# Escreve nos dois pipes em momentos diferentes
loop.add_timer(200) do
  wr1.write('dado-oneshot')
  wr2.write('dado-1')
end

loop.add_timer(400) do
  wr2.write('dado-2')
end

loop.add_timer(600) do
  wr2.write('dado-3')
  persistent.cancel
  puts "[persistente] cancelado manualmente apos 3 leituras"
end

loop.add_timer(800) { loop.stop }

puts 'Testando oneshot vs persistente...'
loop.run
puts 'Loop terminou.'

loop.destroy
[rd1, wr1, rd2, wr2].each(&:close)
