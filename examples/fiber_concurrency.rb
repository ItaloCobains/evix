# frozen_string_literal: true

# Demonstra concorrencia com Fibers: varias tarefas rodando "ao mesmo tempo"
# usando I/O nao-bloqueante via pipes.

require_relative '../lib/evix'

loop = Evix::Loop.new

# Simula uma tarefa assincrona que le de um pipe apos um delay
def async_task(loop, name, delay_ms)
  rd, wr = IO.pipe

  # Timer simula dado chegando apos delay
  loop.add_timer(delay_ms) do
    wr.write("resultado de #{name}")
    wr.close
  end

  loop.spawn do
    wrapped = loop.wrap(rd)
    puts "[#{name}] esperando dado..."
    data = wrapped.read
    puts "[#{name}] recebeu: #{data}"
    wrapped.close
  end
end

# 3 tarefas com delays diferentes, todas concorrentes
async_task(loop, 'tarefa-A', 500)
async_task(loop, 'tarefa-B', 200)
async_task(loop, 'tarefa-C', 800)

puts 'Iniciando 3 tarefas concorrentes...'
start = Time.now
loop.run
elapsed = ((Time.now - start) * 1000).to_i
puts "Todas terminaram em ~#{elapsed}ms (concorrente, nao 1500ms sequencial)"
loop.destroy
