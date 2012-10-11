if (BUILD_LIBNATPMP_TOOLS)
    find_package(gflags)
    add_executable(public-ip public-ip.cpp)
    target_link_libraries(public-ip natpmp ${gflags_LIBRARIES})
    add_executable(create-map create-map.cpp)
    target_link_libraries(create-map natpmp ${gflags_LIBRARIES})
endif (BUILD_LIBNATPMP_TOOLS)

add_library(natpmp STATIC route.cpp op.cpp util.cpp)
