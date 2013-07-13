cmake_minimum_required(VERSION 2.8)

project(mettanode)
include(CTest)

# On mac, use openssl from brew, not the default system one, because it is too old.
# Run 'brew install openssl' to install it.
if (APPLE)
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
	# Set pkg-config path in case pkg-config is installed on the machine.
	set(ENV{PKG_CONFIG_PATH} /usr/local/opt/openssl/lib/pkgconfig)
	# A bug in cmake prevents use of OPENSSL_ROOT_DIR for finding a custom openssl,
	# so we use an internal variable instead. This needs to be fixed in cmake.
	set(_OPENSSL_ROOT_HINTS_AND_PATHS PATHS /usr/local/opt/openssl)
endif (APPLE)

find_package(OpenSSL REQUIRED)
find_package(Qt4 REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

# Export compile commands for ST2 clang plugin.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLANG "Build using clang." ON)
option(CLANG_GIT "[developer] Build using clang-git." OFF)
option(CLANG_ASAN "Use clang's address-sanitizer." ON)
option(CLANG_LTO "[time-consuming] Enable link-time optimization with LLVM." OFF)
option(CLANG_CHECK_DOCS "[developer] Check documentation consistency using -Wdocumentation" ON)

option(TRAVIS_CI "Build on Travis-CI nodes (disables some warnings)" OFF)

if (CLANG)
	set(CMAKE_CXX_FLAGS "-ferror-limit=3 ${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
	if (NOT APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -v")
	endif (NOT APPLE)
	set(_extraFlags)
	if (CLANG_ASAN)
		set(_extraFlags "${_extraFlags} -fsanitize=address")
	endif ()
	if (CLANG_CHECK_DOCS)
		set(_extraFlags "${_extraFlags} -Wdocumentation")
	endif ()
	set(_extraFlags "${_extraFlags} -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-private-field")
	if (CLANG_LTO)
		set(_extraFlags "${_extraFlags} -emit-llvm")
		set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -O4")
	endif ()
	if (TRAVIS_CI)
		set(_extraFlags "${_extraFlags} -Wno-uninitialized") # Qt code in travis-ci nodes is lame.
	endif ()

	if (CLANG_GIT)
		if (NOT LIBCXX_BASE)
			message(FATAL_ERROR "Need to specify non-default libc++ location.")
		endif (NOT LIBCXX_BASE)
		message(STATUS "ENV{DYLD_LIBRARY_PATH} $ENV{DYLD_LIBRARY_PATH}")
		#if (NOT $ENV{DYLD_LIBRARY_PATH})
		#	message(FATAL_ERROR "Did you forget to set custom clang DYLD_LIBRARY_PATH location?")
		#endif (NOT $ENV{DYLD_LIBRARY_PATH})

		# Include local libcxx first.
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -iwithprefixbefore${LIBCXX_BASE}/include/c++/v1")
	endif (CLANG_GIT)

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

if (BUILD_TESTING)
	list(APPEND BOOST_COMPONENTS unit_test_framework)
	enable_testing()
	include(Dart)
endif (BUILD_TESTING)

# For ssu we need boost/asio which depends on libboost_system
list(APPEND BOOST_COMPONENTS system)
# For logging we need boost/posix_time
list(APPEND BOOST_COMPONENTS date_time)
# Serialization is used for streaming data to and from udp packets
list(APPEND BOOST_COMPONENTS serialization)
# Program_options used to parse cmdline args in some tests
list(APPEND BOOST_COMPONENTS program_options)

#set(Boost_USE_MULTITHREAD ON)
set(Boost_USE_STATIC_LIBS ON) # Easier to deploy elsewhere
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

# Additional unofficial boost libraries
include_directories(3rdparty/endian)

# Create and link a test application.
function(create_test NAME EXTRALIBS)
	add_executable(test_${NAME} test_${NAME}.cpp)
	target_link_libraries(test_${NAME} ${EXTRALIBS} ${Boost_LIBRARIES})
	target_link_libraries(test_${NAME} supc++)
	install(TARGETS test_${NAME}
	    RUNTIME DESTINATION tests/unittests)
	add_test(${NAME} test_${NAME})
endfunction(create_test)

add_subdirectory(support)

add_subdirectory(sst)
add_subdirectory(ssu)
add_subdirectory(routing)
#add_subdirectory(naming)
add_subdirectory(nat)
add_subdirectory(ui)

add_subdirectory(playground)
