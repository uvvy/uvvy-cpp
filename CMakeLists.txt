cmake_minimum_required(VERSION 2.8)

project(mettanode)
include(CTest)

include(cmake/setup.cmake)

find_package(Qt4 REQUIRED)

# Export compile commands for ST2 clang plugin.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set flag for submodules build
set(BUILD_IN_METTANODE ON)

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
	set(_extraFlags "${_extraFlags} -Wno-unused-parameter -Wno-unused-private-field")

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

if (APPLE)
	set(GUI_TYPE MACOSX_BUNDLE)
endif (APPLE)

include_directories(3rdparty/endian) # Additional unofficial boost libraries
include_directories(3rdparty/plist/include)

add_subdirectory(3rdparty/plist)
add_subdirectory(support)

include_directories(routing/include) # For ssu access to routing client

#add_subdirectory(sst)
add_subdirectory(ssu)
#add_subdirectory(routing)
#add_subdirectory(naming)
add_subdirectory(nat)
add_subdirectory(ui)

add_subdirectory(playground)
