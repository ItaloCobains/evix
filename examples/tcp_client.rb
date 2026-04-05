# frozen_string_literal: true

# Cliente TCP que conecta no echo server (rode tcp_echo.rb primeiro).
# Demonstra Evix::TCPClient.connect com fibers.

require_relative '../lib/evix'

loop = Evix::Loop.new

loop.spawn do
  conn = Evix::TCPClient.connect(loop, '127.0.0.1', 3000)
  puts 'Conectado ao echo server!'

  messages = ['hello evix', 'fiber-based networking', 'bye']

  messages.each do |msg|
    conn.write("#{msg}\n")
    puts "enviado: #{msg}"
    response = conn.read
    puts "resposta: #{response&.chomp}"
  end

  conn.close
  puts 'Desconectado.'
  loop.stop
rescue Errno::ECONNREFUSED
  puts 'Erro: nao foi possivel conectar. Rode tcp_echo.rb primeiro.'
  loop.stop
end

puts 'Conectando em 127.0.0.1:3000 (rode tcp_echo.rb primeiro)...'
loop.run
loop.destroy
