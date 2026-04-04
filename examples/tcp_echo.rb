require_relative "../lib/evix"
require "socket"

loop = Evix::Loop.new
server = TCPServer.new("127.0.0.1", 3000)

loop.add_io(server.fileno, Evix::IO_READ) do
  ruby_client = server.accept_nonblock
  puts "conexao de #{ruby_client.peeraddr[2]}:#{ruby_client.peeraddr[1]}"

  loop.spawn do
    client = loop.wrap(ruby_client)
    while (data = client.read)
      puts "echo: #{data.chomp}"
      client.write(data)
    end
    puts "cliente desconectou"
    client.close
  end
end

puts "Echo server rodando em 127.0.0.1:3000"
puts "Teste com: nc 127.0.0.1 3000"
loop.run
loop.destroy
server.close
