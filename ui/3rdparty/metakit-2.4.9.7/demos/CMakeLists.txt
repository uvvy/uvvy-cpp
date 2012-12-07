include_directories(../include ../src .)

add_executable(demo demo.cpp)
target_link_libraries(demo mk4)
install(TARGETS demo
	RUNTIME DESTINATION tests)

add_executable(dump dump.cpp)
target_link_libraries(dump mk4)
install(TARGETS dump
	RUNTIME DESTINATION tests)

add_executable(myio myio.cpp)
target_link_libraries(myio mk4)
install(TARGETS myio
	RUNTIME DESTINATION tests)
