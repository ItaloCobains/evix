add_library(evix
    "${CMAKE_CURRENT_SOURCE_DIR}/evix.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/evix.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/evix_internal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/evix_kqueue.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/evix_kqueue.c"
)

target_include_directories(evix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(${ENABLE_WARNINGS})
    target_set_warnings(
        TARGET evix
        ENABLE ${ENABLE_WARNINGS}
        AS_ERRORS ${ENABLE_WARNINGS_AS_ERRORS})
endif()

if(${ENABLE_LTO})
    target_enable_lto(TARGET evix ENABLE ON)
endif()
