# frozen_string_literal: true

# Demonstra a ordem de execucao: callbacks imediatos rodam antes de timers e I/O.

require_relative '../lib/evix'

loop = Evix::Loop.new
order = []

# Timer de 0ms — ainda assim roda depois dos callbacks imediatos
loop.add_timer(0) do
  order << 'timer-0ms'
  puts "3. timer 0ms (#{order.last})"
end

loop.add_timer(100) do
  order << 'timer-100ms'
  puts "4. timer 100ms (#{order.last})"
end

# Callbacks imediatos — rodam primeiro, na ordem de registro
loop.add_callback do
  order << 'callback-1'
  puts "1. callback imediato 1 (#{order.last})"
end

loop.add_callback do
  order << 'callback-2'
  puts "2. callback imediato 2 (#{order.last})"

  # Callback registrado dentro de outro callback
  loop.add_callback do
    order << 'callback-3-nested'
    puts "   callback aninhado (#{order.last})"
  end
end

loop.add_timer(200) do
  puts ''
  puts "Ordem de execucao: #{order.inspect}"
  loop.stop
end

puts 'Testando ordem de execucao...'
puts ''
loop.run
loop.destroy
