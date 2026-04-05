# frozen_string_literal: true

require 'mkmf'

evix_dir = File.expand_path('../evix', __dir__)

$INCFLAGS << " -I#{evix_dir}"
$VPATH << evix_dir

$srcs = ['evix_ruby.c', 'evix.c']

if RUBY_PLATFORM =~ /linux/
  $srcs << 'evix_epoll.c'
  $defs << '-D__linux__'
else
  $srcs << 'evix_kqueue.c'
end

create_makefile('evix_ruby')
