# frozen_string_literal: true

require_relative '../lib/evix'

loop = Evix::Loop.new

# Cria um pipe -- rd fica pronto pra leitura quando alguem escreve em wr
rd, wr = IO.pipe

# Registra I/O watcher no fd de leitura
loop.add_io(rd.fileno, Evix::IO_READ) do
  msg = rd.read_nonblock(1024)
  puts "recebido: #{msg}"
  loop.stop
end

# Timer que escreve no pipe apos 500ms
loop.add_timer(500) do
  wr.write('hello from evix!')
  puts 'escreveu no pipe'
end

puts 'Esperando I/O...'
loop.run
puts 'Loop terminou.'

loop.destroy
rd.close
wr.close
