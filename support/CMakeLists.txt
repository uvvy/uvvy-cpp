# Support library.

include_directories(include)
add_subdirectory(lib)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif (BUILD_TESTING)

