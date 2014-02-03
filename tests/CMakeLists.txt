include_directories(../include ../src .)

set(tests_SOURCES regress.cpp tbasic1.cpp tbasic2.cpp tcusto1.cpp tcusto2.cpp tdiffer.cpp textend.cpp tformat.cpp tlimits.cpp tmapped.cpp tnotify.cpp tresize.cpp tstore1.cpp tstore2.cpp tstore3.cpp tstore4.cpp tstore5.cpp)

add_executable(regress ${tests_SOURCES})
target_link_libraries(regress mk4_static)
install(TARGETS regress
	RUNTIME DESTINATION tests/metakit)

add_test(NAME regression WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.. COMMAND regress)
#test -f reversed || cp ../unix/reversed .

