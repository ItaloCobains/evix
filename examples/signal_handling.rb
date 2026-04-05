# frozen_string_literal: true

# Demonstra tratamento de sinais integrado ao event loop.

require_relative '../lib/evix'

loop = Evix::Loop.new
usr1_count = 0

# Registra handler para SIGUSR1
loop.add_signal('USR1') do
  usr1_count += 1
  puts "SIGUSR1 recebido (##{usr1_count})"
  loop.stop if usr1_count >= 3
end

# Registra handler para SIGINT (Ctrl+C)
loop.add_signal('INT') do
  puts "\nSIGINT recebido — encerrando graciosamente"
  loop.stop
end

# Envia SIGUSR1 para si mesmo 3 vezes, com intervalo
loop.add_timer(300) { Process.kill('USR1', Process.pid) }
loop.add_timer(600) { Process.kill('USR1', Process.pid) }
loop.add_timer(900) { Process.kill('USR1', Process.pid) }

puts "PID: #{Process.pid}"
puts 'Aguardando sinais (ou Ctrl+C para sair)...'
loop.run
puts "Loop terminou. SIGUSR1 recebido #{usr1_count}x."
loop.destroy
