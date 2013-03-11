set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_executable(test_base32 test_base32.cpp)
target_link_libraries(test_base32 support ${Boost_LIBRARIES})
add_executable(test_byte_array test_byte_array.cpp)
target_link_libraries(test_byte_array support ${Boost_LIBRARIES})

install(TARGETS
	test_base32
	test_byte_array
    RUNTIME DESTINATION tests/unittests)

add_test(base32 test_base32)
add_test(byte_array test_byte_array)
