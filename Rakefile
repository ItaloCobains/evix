require "rake/clean"
require "rake/testtask"

BUILD_DIR = "build"

CLEAN.include(BUILD_DIR, "tmp")

# --- C library (shared, for C tests) ---

desc "Compile the C library (shared)"
task :compile do
  sh "cmake -S . -B #{BUILD_DIR} -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release"
  sh "cmake --build #{BUILD_DIR}"
end

desc "Compile and run C tests"
task :test_c do
  sh "cmake -S . -B #{BUILD_DIR} -DBUILD_SHARED_LIBS=ON -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug"
  sh "cmake --build #{BUILD_DIR}"
  sh "cd #{BUILD_DIR} && ctest --output-on-failure"
end

# --- Ruby C extension ---

desc "Compile the Ruby C extension"
task :compile_ext do
  ext_dir = "ext/evix_ruby"
  sh "cd #{ext_dir} && ruby extconf.rb && make"
end

# --- Ruby tests ---

Rake::TestTask.new(:test_ruby) do |t|
  t.libs << "lib"
  t.libs << "ext/evix_ruby"
  t.test_files = FileList["test/ruby/test_*.rb"]
end
task test_ruby: :compile_ext

desc "Run all tests (C + Ruby)"
task test: [:test_c, :test_ruby]

desc "Run Ruby example"
task example: :compile do
  sh "ruby examples/basic.rb"
end

task default: :compile
