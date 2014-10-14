include_directories(../nat/include ../ui) # For traverse_nat.h

add_executable(opus-streaming opus-streaming.cpp)
target_link_libraries(opus-streaming sss arsenal ${VOICEBOX_LIBS} opus rclient routing
    natclient nat upnpc krypto ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
install(TARGETS opus-streaming
    RUNTIME DESTINATION tools)
