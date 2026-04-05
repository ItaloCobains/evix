# frozen_string_literal: true

# Demonstra timers recorrentes e one-shot para parar o loop.

require_relative '../lib/evix'

loop = Evix::Loop.new
count = 0

# Timer recorrente a cada 200ms
ticker = loop.add_timer(200, repeat: 200) do
  count += 1
  puts "[#{count}] tick a cada 200ms"
end

# Timer one-shot que cancela o ticker apos 1s
loop.add_timer(1000) do
  puts "1s passou — cancelando ticker (disparou #{count}x)"
  ticker.cancel
  puts "ticker cancelado? #{ticker.cancelled?}"
end

# Timer que para o loop apos 1.5s (garante que nao ha mais ticks)
loop.add_timer(1500) do
  puts 'nenhum tick extra, parando loop'
  loop.stop
end

puts 'Iniciando timers recorrentes...'
loop.run
puts 'Loop terminou.'
loop.destroy
