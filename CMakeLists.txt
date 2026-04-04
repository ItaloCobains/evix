cmake_minimum_required(VERSION 3.22)

project(
    "evix"
    VERSION 0.1.0
    LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_TESTING "Enable unit tests" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" ON)

# CMAKE MODULES
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(ConfigSafeGuards)

if(ENABLE_WARNINGS)
    include(Warnings)
endif()

if(ENABLE_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif()

if(ENABLE_LTO)
    include(LTO)
endif()

# Library
add_subdirectory(ext/evix)

# Tests
if(ENABLE_TESTING)
    include(CPM)
    cpmaddpackage("gh:ThrowTheSwitch/Unity#v2.5.2")
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

# Install
install(
    TARGETS evix
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

install(
    FILES ext/evix/evix.h
    DESTINATION include)
