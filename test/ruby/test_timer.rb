# frozen_string_literal: true

require_relative 'test_helper'

class TestTimer < Minitest::Test
  def test_timer_fires
    loop = Evix::Loop.new
    fired = false
    loop.add_timer(10) { fired = true }
    loop.run
    assert fired, 'timer should have fired'
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
    assert_equal %i[first second third], order
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

  def test_recurring_timer
    loop = Evix::Loop.new
    count = 0
    loop.add_timer(20, repeat: 20) { count += 1 }
    loop.add_timer(70) { loop.stop }
    loop.run
    assert count >= 3, "recurring timer should fire at least 3 times, got #{count}"
    loop.destroy
  end

  def test_add_timer_returns_timer_handle
    loop = Evix::Loop.new
    timer = loop.add_timer(10) { nil }
    assert_instance_of Evix::Timer, timer
    loop.destroy
  end

  def test_timer_cancel
    loop = Evix::Loop.new
    fired = false
    timer = loop.add_timer(20) { fired = true }
    timer.cancel
    loop.add_timer(50) { loop.stop }
    loop.run
    refute fired, 'cancelled timer should not fire'
    assert timer.cancelled?
    loop.destroy
  end

  def test_cancel_recurring_timer
    loop = Evix::Loop.new
    count = 0
    timer = loop.add_timer(10, repeat: 10) { count += 1 }
    loop.add_timer(35) { timer.cancel }
    loop.add_timer(60) { loop.stop }
    loop.run
    assert count >= 2, "should have fired before cancel, got #{count}"
    assert count <= 4, "should have stopped after cancel, got #{count}"
    assert timer.cancelled?
    loop.destroy
  end

  def test_oneshot_timer_auto_marks_cancelled
    loop = Evix::Loop.new
    timer = loop.add_timer(10) { nil }
    loop.run
    assert timer.cancelled?, 'one-shot timer should be marked cancelled after firing'
    loop.destroy
  end

  def test_timer_with_callback
    loop = Evix::Loop.new
    order = []
    loop.add_callback { order << :callback }
    loop.add_timer(10) { order << :timer }
    loop.run
    assert_equal %i[callback timer], order
    loop.destroy
  end
end
