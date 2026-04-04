require "rake/clean"

BUILD_DIR = "build"

CLEAN.include(BUILD_DIR)

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

desc "Run Ruby example"
task :example => :compile do
  sh "ruby examples/basic.rb"
end

task default: :compile
