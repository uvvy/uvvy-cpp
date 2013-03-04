cmake_minimum_required(VERSION 2.8)

project(mettanode)

find_package(Qt4 REQUIRED)
#find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

option(BUILD_TESTS "Build various tests and test programs." OFF)
option(CLANG "Build using clang." OFF)
option(CHECK_DOCS "Check documentation consistency using clang's -Wdocumentation" ON)

if (CLANG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdocumentation -std=c++11 -fsanitize=address")
endif (CLANG)

if (CHECK_DOCS)
	set(_docflag "-Wdocumentation")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_docflag}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_docflag}")
endif (CHECK_DOCS)

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

if (APPLE)
	set(GUI_TYPE MACOSX_BUNDLE)
endif (APPLE)

if (BUILD_TESTS)
	enable_testing()
	include(Dart)
endif (BUILD_TESTS)

add_subdirectory(sst)
add_subdirectory(routing)
#add_subdirectory(naming)
add_subdirectory(nat)
add_subdirectory(ui)

add_subdirectory(playground)
