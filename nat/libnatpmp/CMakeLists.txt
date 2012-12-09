option(BUILD_LIBNATPMP_TOOLS "Build nat-pmp applications (requires gflags, 'brew install gflags')" OFF)

if (BUILD_LIBNATPMP_TOOLS)
	find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h)
	find_library(GFLAGS_LIBRARY NAMES gflags)

	include_directories(GFLAGS_INCLUDE_DIR)

    add_executable(public-ip public-ip.cpp)
    target_link_libraries(public-ip natpmp ${GFLAGS_LIBRARY})
    add_executable(create-map create-map.cpp)
    target_link_libraries(create-map natpmp ${GFLAGS_LIBRARY})
endif (BUILD_LIBNATPMP_TOOLS)

add_library(natpmp STATIC route.cpp op.cpp util.cpp)
