require_relative "test_helper"

class TestLoop < Minitest::Test
  def setup
    @loop = Evix::Loop.new(backend: nil)
  end

  def teardown
    @loop.destroy
  end

  def test_create_and_destroy
    loop = Evix::Loop.new
    loop.destroy
  end

  def test_create_with_kqueue_backend
    loop = Evix::Loop.new(backend: :kqueue)
    loop.destroy
  end

  def test_create_without_backend
    loop = Evix::Loop.new(backend: nil)
    loop.destroy
  end

  def test_empty_loop_returns_immediately
    @loop.run
  end

  def test_single_callback
    called = false
    @loop.add_callback { called = true }
    @loop.run
    assert called, "callback should have been called"
  end

  def test_multiple_callbacks_in_order
    order = []
    @loop.add_callback { order << 1 }
    @loop.add_callback { order << 2 }
    @loop.add_callback { order << 3 }
    @loop.run
    assert_equal [1, 2, 3], order
  end

  def test_callback_can_enqueue_another
    order = []
    @loop.add_callback do
      order << :first
      @loop.add_callback { order << :second }
    end
    @loop.run
    assert_equal [:first, :second], order
  end

  def test_stop_halts_loop
    loop = Evix::Loop.new
    fired = []
    loop.add_timer(10) do
      fired << :timer
      loop.stop
    end
    loop.add_timer(500) { fired << :late }
    loop.run
    assert_equal [:timer], fired
    loop.destroy
  end

  def test_destroy_is_idempotent
    loop = Evix::Loop.new(backend: nil)
    loop.destroy
    loop.destroy
  end

  def test_use_after_destroy_raises
    loop = Evix::Loop.new(backend: nil)
    loop.destroy
    assert_raises(RuntimeError) { loop.run }
  end

  def test_add_callback_without_block_raises
    assert_raises(ArgumentError) { @loop.add_callback }
  end

  def test_add_timer_without_block_raises
    assert_raises(ArgumentError) { @loop.add_timer(10) }
  end

  def test_add_io_without_block_raises
    rd, wr = IO.pipe
    assert_raises(ArgumentError) { @loop.add_io(rd.fileno) }
    rd.close
    wr.close
  end

  def test_invalid_backend_raises
    assert_raises(ArgumentError) { Evix::Loop.new(backend: :epoll) }
  end
end
