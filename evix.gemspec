require_relative "lib/evix/version"

Gem::Specification.new do |s|
  s.name        = "evix"
  s.version     = Evix::VERSION
  s.summary     = "Lightweight event loop for Ruby, powered by a C core"
  s.description = "Evix is a minimal event loop library written in C with native Ruby bindings. " \
                  "Supports immediate callbacks, one-shot timers, I/O watchers, and Fiber-based async I/O."
  s.authors     = ["Italo Brandao"]
  s.license     = "MIT"
  s.homepage    = "https://github.com/italobrandao/evix"

  s.required_ruby_version = ">= 3.0"

  s.files = Dir[
    "lib/**/*.rb",
    "ext/**/*.{c,h,rb}",
    "ext/**/CMakeLists.txt",
    "cmake/**/*.cmake",
    "CMakeLists.txt",
    "LICENSE",
    "README.md"
  ]

  s.require_paths = ["lib"]
  s.extensions    = ["ext/evix_ruby/extconf.rb"]

  s.add_development_dependency "minitest", "~> 5.0"
  s.add_development_dependency "rake", "~> 13.0"
end
