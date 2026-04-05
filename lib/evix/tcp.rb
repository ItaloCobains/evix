# frozen_string_literal: true

require 'socket'

module Evix
  # Fiber-based TCP server that spawns a fiber per connection.
  class TCPServer
    def initialize(loop, host, port, &handler)
      @loop = loop
      @server = ::TCPServer.new(host, port)
      @server.setsockopt(Socket::SOL_SOCKET, Socket::SO_REUSEADDR, true)
      @handler = handler

      @loop.add_io(@server.fileno, IO_READ) do
        accept_connections
      end
    end

    def close
      @server.close unless @server.closed?
    end

    private

    def accept_connections
      loop do
        client = @server.accept_nonblock
        @loop.spawn do
          wrapped = @loop.wrap(client)
          @handler.call(wrapped)
        ensure
          wrapped&.close
        end
      rescue ::IO::WaitReadable
        break
      end
    end
  end

  # Fiber-based TCP client. Must be called from within a spawned fiber.
  module TCPClient
    def self.connect(loop, host, port)
      socket = Socket.new(:INET, :STREAM)
      addr = Socket.sockaddr_in(port, host)

      begin
        socket.connect_nonblock(addr)
      rescue ::IO::WaitWritable
        fiber = Fiber.current
        loop.add_io(socket.fileno, IO_WRITE | IO_ONESHOT) { fiber.resume }
        Fiber.yield

        err = socket.getsockopt(Socket::SOL_SOCKET, Socket::SO_ERROR).int
        raise Errno::ECONNREFUSED, "#{host}:#{port}" unless err.zero?
      end

      Evix::IO.new(loop, socket)
    end
  end
end
