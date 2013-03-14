cmake_minimum_required(VERSION 2.8)

project(mettanode)

find_package(Qt4 REQUIRED)
#find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

# Export compile commands for ST2 clang plugin.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build various tests and test programs." OFF)
option(CLANG "Build using clang." ON)
option(CLANG_ASAN "Use clang's address-sanitizer." ON)
option(CLANG_LTO "[time-consuming] Enable link-time optimization with LLVM." OFF)
option(CLANG_CHECK_DOCS "Check documentation consistency using clang's -Wdocumentation" ON)

if (CLANG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
	set(_extraFlags)
	if (CLANG_ASAN)
		set(_extraFlags "${_extraFlags} -fsanitize=address")
	endif ()
	if (CLANG_CHECK_DOCS)
		set(_extraFlags "${_extraFlags} -Wdocumentation")
	endif ()
	if (CLANG_LTO)
		set(_extraFlags "${_extraFlags} -emit-llvm")
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -O4")
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_extraFlags}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_extraFlags}")
endif (CLANG)

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

set(BOOST_COMPONENTS)

if (BUILD_TESTS)
	list(APPEND BOOST_COMPONENTS unit_test_framework)
	enable_testing()
	include(Dart)
endif (BUILD_TESTS)

# For ssu we need boost/asio which depends on libboost_system
list(APPEND BOOST_COMPONENTS system)
# For logging we need boost/posix_time
list(APPEND BOOST_COMPONENTS date_time)
# Serialization is used for streaming data to and from udp packets
list(APPEND BOOST_COMPONENTS serialization)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

add_subdirectory(support)

add_subdirectory(sst)
add_subdirectory(ssu)
add_subdirectory(routing)
#add_subdirectory(naming)
add_subdirectory(nat)
add_subdirectory(ui)

add_subdirectory(playground)
