# frozen_string_literal: true

# Chat server simples: broadcast de mensagens para todos os clientes conectados.
# Teste com multiplos terminais: nc 127.0.0.1 3001

require_relative '../lib/evix'
require 'socket'

loop = Evix::Loop.new
clients = {}

server = Evix::TCPServer.new(loop, '127.0.0.1', 3001) do |client|
  addr = "client-#{clients.size + 1}"
  clients[addr] = client
  puts "#{addr} conectou (#{clients.size} online)"

  begin
    client.write("Bem-vindo, #{addr}! (#{clients.size} online)\n")

    while (data = client.read)
      msg = "#{addr}: #{data}"
      print msg
      clients.each do |name, c|
        next if name == addr

        begin
          c.write(msg)
        rescue StandardError
          nil
        end
      end
    end
  ensure
    clients.delete(addr)
    client.close
    puts "#{addr} desconectou (#{clients.size} online)"
  end
end

# Handler para Ctrl+C
loop.add_signal('INT') do
  puts "\nEncerrando servidor..."
  server.close
  loop.stop
end

puts 'Chat server rodando em 127.0.0.1:3001'
puts 'Teste com: nc 127.0.0.1 3001 (varios terminais)'
puts 'Ctrl+C para sair'
loop.run
loop.destroy
