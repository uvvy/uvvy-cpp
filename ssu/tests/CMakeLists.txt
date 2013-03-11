set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_wwwww test_wwww.cpp)
target_link_libraries(test_wwww ssu ${Boost_LIBRARIES})
install(TARGETS test_wwww
    RUNTIME DESTINATION tests/unittests)

add_test(wwww test_wwww)
