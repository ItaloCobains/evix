require "ffi"

module Evix
  module FFIBindings
    extend FFI::Library

    LIB_EXT = case RbConfig::CONFIG["host_os"]
              when /darwin/ then "dylib"
              when /linux/  then "so"
              else raise "Unsupported OS: #{RbConfig::CONFIG["host_os"]}"
              end

    LIB_PATH = File.expand_path("../../build/ext/evix/libevix.#{LIB_EXT}", __dir__)

    ffi_lib LIB_PATH

    callback :evix_callback_fn, [:pointer], :void

    # Loop lifecycle
    attach_function :evix_loop_create,       [:pointer], :pointer
    attach_function :evix_loop_destroy,      [:pointer], :void
    attach_function :evix_loop_run,          [:pointer], :int
    attach_function :evix_loop_stop,         [:pointer], :void
    attach_function :evix_loop_add_callback, [:pointer, :evix_callback_fn, :pointer], :void

    # Timers
    attach_function :evix_timer_create,  [:pointer, :uint64, :evix_callback_fn, :pointer], :pointer
    attach_function :evix_timer_destroy, [:pointer], :void

    # I/O watchers
    attach_function :evix_io_create, [:pointer, :int, :int, :evix_callback_fn, :pointer], :pointer
    attach_function :evix_io_stop,   [:pointer, :pointer], :void

    # Kqueue backend
    attach_function :evix_kqueue_backend, [], :pointer
  end
end
