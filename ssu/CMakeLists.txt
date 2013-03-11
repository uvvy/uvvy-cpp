include_directories(include)
add_subdirectory(doc)
add_subdirectory(lib)

if (BUILD_TESTS)
    #add_subdirectory(tests)
endif (BUILD_TESTS)
