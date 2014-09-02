include_directories(../nat/include ../ssu/include ../arsenal/include)
include_directories(../3rdparty/opus/include) # For unlogger
include_directories(../3rdparty) # For cryptest

add_executable(unlogger unlogger.cpp)
target_link_libraries(unlogger arsenal opus ${Boost_LIBRARIES})
install(TARGETS unlogger
    RUNTIME DESTINATION tools)

#if (APPLE)
#    add_executable(screencaster screencaster.cpp) #macosx.mm
#    target_link_libraries(screencaster ${QT_LIBRARIES} objc)
#    install(TARGETS screencaster
#        RUNTIME DESTINATION tools)
#endif (APPLE)

#add_subdirectory(sstreams)

add_executable(cryptest cryptest.cpp)
target_link_libraries(cryptest sodiumpp arsenal)
