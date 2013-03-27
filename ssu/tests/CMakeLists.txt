function(create_test NAME)
	add_executable(test_${NAME} test_${NAME}.cpp)
	target_link_libraries(test_${NAME} ssu support ${Boost_LIBRARIES})
	install(TARGETS test_${NAME}
	    RUNTIME DESTINATION tests/unittests)
	add_test(${NAME} test_${NAME})
endfunction(create_test)

create_test(link)
create_test(host)
create_test(channel)
create_test(stream_user)
create_test(stream_internal)

create_test(run_udp_receiver)

create_test(optional)
create_test(serialization)
