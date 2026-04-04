require_relative "test_helper"

class TestTimer < Minitest::Test
  def test_timer_fires
    loop = Evix::Loop.new
    fired = false
    loop.add_timer(10) { fired = true }
    loop.run
    assert fired, "timer should have fired"
    loop.destroy
  end

  def test_timer_fires_after_delay
    loop = Evix::Loop.new
    t0 = Process.clock_gettime(Process::CLOCK_MONOTONIC, :millisecond)
    elapsed = nil
    loop.add_timer(50) do
      elapsed = Process.clock_gettime(Process::CLOCK_MONOTONIC, :millisecond) - t0
    end
    loop.run
    assert elapsed >= 40, "timer should have waited ~50ms, got #{elapsed}ms"
    loop.destroy
  end

  def test_timers_fire_in_order
    loop = Evix::Loop.new
    order = []
    loop.add_timer(60) { order << :second }
    loop.add_timer(20) { order << :first }
    loop.add_timer(100) { order << :third }
    loop.run
    assert_equal [:first, :second, :third], order
    loop.destroy
  end

  def test_zero_delay_timer
    loop = Evix::Loop.new
    fired = false
    loop.add_timer(0) { fired = true }
    loop.run
    assert fired
    loop.destroy
  end

  def test_timer_with_callback
    loop = Evix::Loop.new
    order = []
    loop.add_callback { order << :callback }
    loop.add_timer(10) { order << :timer }
    loop.run
    assert_equal [:callback, :timer], order
    loop.destroy
  end
end
