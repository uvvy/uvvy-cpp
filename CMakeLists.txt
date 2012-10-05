cmake_minimum_required(VERSION 2.8)

project(metta-grabber)

find_package(Qt4 REQUIRED)
find_package(boost REQUIRED)
find_package(openssl REQUIRED)


# How to run rpcgen to build XDR stubs
macro(WRAP_RPC_SOURCES name sources)

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
GET_TARGET_PROPERTY(RPCGEN_EXE rpcgen LOCATION) # should not be needed past cmake 2.4 if this file knows about rpcgen target...
foreach (src ${sources})
	string(REGEX REPLACE ".x$" "" tgt ${src})
	ADD_CUSTOM_COMMAND(OUTPUT
	    ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp
	    COMMAND ${RPCGEN_EXE} -h -o ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h    ${CMAKE_CURRENT_SOURCE_DIR}/${src}
	    COMMAND ${RPCGEN_EXE} -c -o ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/${src}
	    DEPENDS ${RPCGEN_EXE}
	    MAIN_DEPENDENCY ${src})
	list(APPEND ${name} ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.cpp ${CMAKE_CURRENT_BINARY_DIR}/${tgt}.h)
endforeach()
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

endmacro(WRAP_RPC_SOURCES)



add_subdirectory(sst)
#add_subdirectory(ui)
