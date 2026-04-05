# frozen_string_literal: true

require_relative 'test_helper'

class TestIO < Minitest::Test
  def test_io_read_with_pipe
    loop = Evix::Loop.new
    rd, wr = IO.pipe

    received = nil
    loop.add_io(rd.fileno, Evix::IO_READ | Evix::IO_ONESHOT) do
      received = rd.read_nonblock(64)
      loop.stop
    end

    loop.add_timer(10) { wr.write('hello') }
    loop.run

    assert_equal 'hello', received
    rd.close
    wr.close
    loop.destroy
  end

  def test_io_write_with_pipe
    loop = Evix::Loop.new
    rd, wr = IO.pipe

    written = false
    loop.add_io(wr.fileno, Evix::IO_WRITE | Evix::IO_ONESHOT) do
      wr.write_nonblock('data')
      written = true
      loop.stop
    end

    loop.run

    assert written
    assert_equal 'data', rd.read_nonblock(64)
    rd.close
    wr.close
    loop.destroy
  end

  def test_io_oneshot_fires_once
    loop = Evix::Loop.new
    rd, wr = IO.pipe
    count = 0

    loop.add_io(rd.fileno, Evix::IO_READ | Evix::IO_ONESHOT) do
      rd.read_nonblock(64)
      count += 1
    end

    loop.add_timer(10) { wr.write('first') }
    loop.add_timer(30) { wr.write('second') }
    loop.add_timer(60) { loop.stop }
    loop.run

    assert_equal 1, count, 'oneshot should fire only once'
    rd.close
    wr.close
    loop.destroy
  end

  def test_io_persistent_fires_multiple_times
    loop = Evix::Loop.new
    rd, wr = IO.pipe
    count = 0

    loop.add_io(rd.fileno, Evix::IO_READ) do
      rd.read_nonblock(64)
      count += 1
      loop.stop if count >= 3
    end

    loop.add_timer(10) { wr.write('a') }
    loop.add_timer(30) { wr.write('b') }
    loop.add_timer(50) { wr.write('c') }
    loop.run

    assert_equal 3, count, 'persistent watcher should fire multiple times'
    rd.close
    wr.close
    loop.destroy
  end

  def test_io_default_events_is_read
    loop = Evix::Loop.new
    rd, wr = IO.pipe

    received = nil
    loop.add_io(rd.fileno) do
      received = rd.read_nonblock(64)
      loop.stop
    end

    loop.add_timer(10) { wr.write('default') }
    loop.run

    assert_equal 'default', received
    rd.close
    wr.close
    loop.destroy
  end

  def test_add_io_returns_io_watcher_handle
    loop = Evix::Loop.new
    rd, wr = IO.pipe
    watcher = loop.add_io(rd.fileno, Evix::IO_READ | Evix::IO_ONESHOT) { nil }
    assert_instance_of Evix::IOWatcher, watcher
    rd.close
    wr.close
    loop.destroy
  end

  def test_io_watcher_cancel
    loop = Evix::Loop.new
    rd, wr = IO.pipe
    fired = false
    watcher = loop.add_io(rd.fileno, Evix::IO_READ | Evix::IO_ONESHOT) { fired = true }
    watcher.cancel
    loop.add_timer(10) { wr.write('data') }
    loop.add_timer(30) { loop.stop }
    loop.run
    refute fired, 'cancelled watcher should not fire'
    assert watcher.cancelled?
    rd.close
    wr.close
    loop.destroy
  end

  def test_constants_defined
    assert_equal 1, Evix::IO_READ
    assert_equal 2, Evix::IO_WRITE
    assert_equal 4, Evix::IO_ONESHOT
  end
end
