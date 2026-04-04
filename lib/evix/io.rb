module Evix
  class IO
    def initialize(loop, ruby_io)
      @loop = loop
      @ruby_io = ruby_io
      @fd = ruby_io.fileno
    end

    def read(maxlen = 4096)
      wait_readable
      @ruby_io.read_nonblock(maxlen)
    rescue ::IO::WaitReadable
      retry
    rescue EOFError
      nil
    end

    def write(data)
      remaining = data
      while remaining && !remaining.empty?
        begin
          n = @ruby_io.write_nonblock(remaining)
          remaining = remaining[n..]
        rescue ::IO::WaitWritable
          wait_writable
          retry
        end
      end
    end

    def close
      @ruby_io.close unless @ruby_io.closed?
    end

    def closed?
      @ruby_io.closed?
    end

    private

    def wait_readable
      fiber = Fiber.current
      @loop.add_io(@fd, IO_READ | IO_ONESHOT) { fiber.resume }
      Fiber.yield
    end

    def wait_writable
      fiber = Fiber.current
      @loop.add_io(@fd, IO_WRITE | IO_ONESHOT) { fiber.resume }
      Fiber.yield
    end
  end
end
