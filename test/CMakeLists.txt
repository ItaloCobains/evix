add_executable(test_evix_loop test_evix_loop.c)
target_link_libraries(test_evix_loop PRIVATE evix unity)
add_test(NAME test_evix_loop COMMAND test_evix_loop)

add_executable(test_evix_backend test_evix_backend.c)
target_link_libraries(test_evix_backend PRIVATE evix unity)
add_test(NAME test_evix_backend COMMAND test_evix_backend)

add_executable(test_evix_kqueue test_evix_kqueue.c)
target_link_libraries(test_evix_kqueue PRIVATE evix unity)
add_test(NAME test_evix_kqueue COMMAND test_evix_kqueue)

if(${ENABLE_WARNINGS})
    target_set_warnings(
        TARGET test_evix_loop
        ENABLE ${ENABLE_WARNINGS}
        AS_ERRORS ${ENABLE_WARNINGS_AS_ERRORS})
endif()

if(ENABLE_COVERAGE)
    set(COVERAGE_MAIN "coverage")
    set(COVERAGE_EXCLUDES
        "${PROJECT_SOURCE_DIR}/cmake/*"
        "${PROJECT_SOURCE_DIR}/test/*"
        "${PROJECT_SOURCE_DIR}/build/*"
        "/usr/include/*")
    set(COVERAGE_DEPENDENCIES test_evix_loop)

    setup_target_for_coverage_gcovr_html(
        NAME ${COVERAGE_MAIN}
        EXECUTABLE ctest
        DEPENDENCIES ${COVERAGE_DEPENDENCIES}
        BASE_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
