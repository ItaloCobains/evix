require_relative "../lib/evix"

loop = Evix::Loop.new

loop.add_callback { puts "callback imediato 1" }
loop.add_callback { puts "callback imediato 2" }

loop.add_timer(500)  { puts "timer 500ms disparou!" }
loop.add_timer(1000) { puts "timer 1s disparou!" }
loop.add_timer(200)  { puts "timer 200ms disparou!" }

puts "Iniciando o event loop..."
loop.run
puts "Event loop terminou."

loop.destroy
