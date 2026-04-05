# frozen_string_literal: true

require_relative 'evix/version'
require 'evix_ruby'
require_relative 'evix/io'
require_relative 'evix/tcp'

module Evix
  # Event loop with Fiber-based concurrency and async I/O.
  class Loop
    alias_method :c_run, :run
    alias_method :c_destroy, :destroy

    def spawn(&block)
      fiber = Fiber.new do
        block.call
      rescue StandardError => e
        @spawn_error ||= e
        stop
      end
      add_callback { fiber.resume }
    end

    def run
      @spawn_error = nil
      result = c_run
      raise @spawn_error if @spawn_error

      result
    end

    def wrap(ruby_io)
      Evix::IO.new(self, ruby_io)
    end

    def add_signal(sig, &block)
      rd, wr = ::IO.pipe
      @signal_pipes ||= []
      prev = Signal.trap(sig) { wr.write_nonblock("\x00") rescue nil } # rubocop:disable Style/RescueModifier
      @signal_pipes << { rd: rd, wr: wr, sig: sig, prev: prev }

      add_io(rd.fileno) do
        rd.read_nonblock(256) rescue nil # rubocop:disable Style/RescueModifier
        block.call
      end
    end

    def destroy
      cleanup_signals
      c_destroy
    end

    private

    def cleanup_signals
      return unless @signal_pipes

      @signal_pipes.each do |s|
        Signal.trap(s[:sig], s[:prev] || 'DEFAULT')
        s[:rd].close rescue nil # rubocop:disable Style/RescueModifier
        s[:wr].close rescue nil # rubocop:disable Style/RescueModifier
      end
      @signal_pipes = nil
    end
  end
end
