require_relative "evix/version"
require_relative "evix/ffi"

module Evix
  class Loop
    def initialize
      @ptr = FFIBindings.evix_loop_create
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
