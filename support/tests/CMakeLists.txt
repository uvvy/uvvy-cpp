add_executable(test_base32 test_base32.cpp)
target_link_libraries(test_base32 support ${Boost_LIBRARIES})
add_executable(test_byte_array test_byte_array.cpp)
target_link_libraries(test_byte_array support ${Boost_LIBRARIES})
add_executable(test_logging test_logging.cpp)
target_link_libraries(test_logging support ${Boost_LIBRARIES})

install(TARGETS
	test_base32
	test_byte_array
	test_logging
    RUNTIME DESTINATION tests/unittests)

add_test(base32 test_base32)
add_test(byte_array test_byte_array)
add_test(logging test_logging)
