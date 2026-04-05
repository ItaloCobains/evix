# frozen_string_literal: true

# Demonstra um pipeline de processamento usando fibers e pipes:
# produtor -> transformador -> consumidor

require_relative '../lib/evix'

loop = Evix::Loop.new

# Cria dois pipes para o pipeline
pipe1_rd, pipe1_wr = IO.pipe
pipe2_rd, pipe2_wr = IO.pipe

# Produtor: gera numeros a cada 150ms
loop.spawn do
  out = loop.wrap(pipe1_wr)
  5.times do |i|
    # Usa timer para simular delay entre producoes
    rd_delay, wr_delay = IO.pipe
    loop.add_timer(150) { wr_delay.write('x'); wr_delay.close }
    delay = loop.wrap(rd_delay)
    delay.read
    delay.close

    valor = (i + 1) * 10
    puts "[produtor] gerando: #{valor}"
    out.write("#{valor}\n")
  end
  out.close
  puts '[produtor] finalizou'
end

# Transformador: dobra os valores
loop.spawn do
  input = loop.wrap(pipe1_rd)
  output = loop.wrap(pipe2_wr)

  while (data = input.read)
    data.split("\n").each do |line|
      next if line.empty?

      valor = line.to_i * 2
      puts "[transformador] #{line} -> #{valor}"
      output.write("#{valor}\n")
    end
  end

  input.close
  output.close
  puts '[transformador] finalizou'
end

# Consumidor: imprime resultado final
results = []
loop.spawn do
  input = loop.wrap(pipe2_rd)

  while (data = input.read)
    data.split("\n").each do |line|
      next if line.empty?

      results << line.to_i
      puts "[consumidor] resultado final: #{line}"
    end
  end

  input.close
  puts "[consumidor] finalizou — total: #{results.sum}"
end

puts 'Pipeline: produtor -> transformador (x2) -> consumidor'
puts ''
loop.run
puts ''
puts "Resultados: #{results.inspect}"
loop.destroy
