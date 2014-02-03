include_directories(../include ../src .)

add_executable(demo demo.cpp)
target_link_libraries(demo mk4_static)

add_executable(dump dump.cpp)
target_link_libraries(dump mk4_static)

add_executable(myio myio.cpp)
target_link_libraries(myio mk4_static)
