require_relative "evix/version"
require_relative "evix/ffi"
require_relative "evix/io"

module Evix
  IO_READ    = 1
  IO_WRITE   = 2
  IO_ONESHOT = 4

  class Loop
    def initialize(backend: :kqueue)
      backend_ptr = case backend
                    when :kqueue then FFIBindings.evix_kqueue_backend
                    when nil     then ::FFI::Pointer::NULL
                    else backend
                    end

      @ptr = FFIBindings.evix_loop_create(backend_ptr)
      raise "Failed to create evix loop" if @ptr.null?
      @refs = []
    end

    def add_callback(&block)
      cb = ::FFI::Function.new(:void, [:pointer]) { |_| block.call }
      @refs << cb
      FFIBindings.evix_loop_add_callback(@ptr, cb, ::FFI::Pointer::NULL)
    end

    def add_timer(delay_ms, &block)
      cb = ::FFI::Function.new(:void, [:pointer]) { |_| block.call }
      @refs << cb
      FFIBindings.evix_timer_create(@ptr, delay_ms, cb, ::FFI::Pointer::NULL)
    end

    def add_io(fd, events = IO_READ, &block)
      cb = ::FFI::Function.new(:void, [:pointer]) { |_| block.call }
      @refs << cb
      FFIBindings.evix_io_create(@ptr, fd, events, cb, ::FFI::Pointer::NULL)
    end

    def spawn(&block)
      fiber = Fiber.new { block.call }
      add_callback { fiber.resume }
    end

    def wrap(ruby_io)
      Evix::IO.new(self, ruby_io)
    end

    def stop
      FFIBindings.evix_loop_stop(@ptr)
    end

    def run
      FFIBindings.evix_loop_run(@ptr)
    end

    def destroy
      return unless @ptr && !@ptr.null?
      FFIBindings.evix_loop_destroy(@ptr)
      @ptr = nil
      @refs.clear
    end
  end
end
