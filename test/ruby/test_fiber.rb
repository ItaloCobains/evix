require_relative "test_helper"

class TestFiber < Minitest::Test
  def test_spawn_executes_block
    loop = Evix::Loop.new(backend: nil)
    executed = false
    loop.spawn { executed = true }
    loop.run
    assert executed
    loop.destroy
  end

  def test_spawn_multiple_fibers
    loop = Evix::Loop.new(backend: nil)
    order = []
    loop.spawn { order << :a }
    loop.spawn { order << :b }
    loop.spawn { order << :c }
    loop.run
    assert_equal [:a, :b, :c], order
    loop.destroy
  end

  def test_fiber_async_io_with_pipe
    loop = Evix::Loop.new
    rd, wr = IO.pipe

    received = nil
    loop.spawn do
      wrapped = loop.wrap(rd)
      received = wrapped.read(64)
      loop.stop
    end

    loop.add_timer(10) { wr.write("fiber hello") }
    loop.run

    assert_equal "fiber hello", received
    rd.close
    wr.close
    loop.destroy
  end

  def test_fiber_async_write
    loop = Evix::Loop.new
    rd, wr = IO.pipe

    loop.spawn do
      wrapped = loop.wrap(wr)
      wrapped.write("from fiber")
      loop.stop
    end

    loop.run

    assert_equal "from fiber", rd.read_nonblock(64)
    rd.close
    wr.close
    loop.destroy
  end

  def test_wrap_close
    loop = Evix::Loop.new(backend: nil)
    rd, wr = IO.pipe
    wrapped = loop.wrap(rd)

    refute wrapped.closed?
    wrapped.close
    assert wrapped.closed?

    wr.close
    loop.destroy
  end

  def test_two_concurrent_fibers_with_io
    loop = Evix::Loop.new
    rd1, wr1 = IO.pipe
    rd2, wr2 = IO.pipe

    results = []
    done = 0

    loop.spawn do
      wrapped = loop.wrap(rd1)
      results << wrapped.read(64)
      done += 1
      loop.stop if done == 2
    end

    loop.spawn do
      wrapped = loop.wrap(rd2)
      results << wrapped.read(64)
      done += 1
      loop.stop if done == 2
    end

    loop.add_timer(10) { wr1.write("from pipe 1") }
    loop.add_timer(20) { wr2.write("from pipe 2") }
    loop.run

    assert_includes results, "from pipe 1"
    assert_includes results, "from pipe 2"
    assert_equal 2, results.size

    [rd1, wr1, rd2, wr2].each(&:close)
    loop.destroy
  end
end
