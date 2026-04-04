# frozen_string_literal: true

require_relative 'evix/version'
require 'evix_ruby'
require_relative 'evix/io'

module Evix
  # Event loop with Fiber-based concurrency and async I/O.
  class Loop
    def spawn(&block)
      fiber = Fiber.new { block.call }
      add_callback { fiber.resume }
    end

    def wrap(ruby_io)
      Evix::IO.new(self, ruby_io)
    end
  end
end
