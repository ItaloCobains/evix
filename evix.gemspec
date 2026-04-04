require_relative "lib/evix/version"

Gem::Specification.new do |s|
  s.name        = "evix"
  s.version     = Evix::VERSION
  s.summary     = "Lightweight event loop for Ruby, powered by a C core"
  s.description = "Evix is a minimal event loop library written in C with Ruby bindings via FFI. " \
                  "Supports immediate callbacks and one-shot timers."
  s.authors     = ["Italo Brandao"]
  s.license     = "MIT"
  s.homepage    = "https://github.com/italobrandao/evix"

  s.required_ruby_version = ">= 3.0"

  s.files = Dir[
    "lib/**/*.rb",
    "ext/**/*.{c,h}",
    "ext/**/CMakeLists.txt",
    "cmake/**/*.cmake",
    "CMakeLists.txt",
    "LICENSE",
    "README.md"
  ]

  s.require_paths = ["lib"]

  s.add_dependency "ffi", "~> 1.15"

  s.extensions = ["ext/Rakefile"]
end
