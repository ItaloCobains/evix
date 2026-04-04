require "mkmf"

evix_dir = File.expand_path("../evix", __dir__)

$INCFLAGS << " -I#{evix_dir}"
$VPATH << evix_dir
$srcs = ["evix_ruby.c", "evix.c", "evix_kqueue.c"]

create_makefile("evix_ruby")
