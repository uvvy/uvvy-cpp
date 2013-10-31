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
option(ASAN_PARANOIA "Use clang's address-sanitizer FULL ON." OFF)
option(CLANG_TSAN "Use clang's thread-sanitizer." OFF)
option(CLANG_MSAN "Use clang's memory-sanitizer." OFF)
option(CLANG_USAN "Use clang's UB-sanitizer." OFF)
option(CLANG_LTO "[time-consuming] Enable link-time optimization with LLVM." OFF)
option(CLANG_CHECK_DOCS "[developer] Check documentation consistency using -Wdocumentation" ON)
option(CLANG_ANALYZE "[developer] Run clang in static analyzer mode." OFF)

option(TRAVIS_CI "Build on Travis-CI nodes (disables some warnings)" OFF)

if (CLANG)
	set(CMAKE_CXX_FLAGS "-ferror-limit=3 ${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
	if (NOT APPLE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -v")
	endif (NOT APPLE)
	set(_extraFlags)
	if (CLANG_ANALYZE)
		set(_extraFlags "${_extraFlags} --analyze -Xanalyzer -analyzer-output=text")
	endif ()
	if (CLANG_ASAN)
		if (ASAN_PARANOIA)
			set(_extraFlags "${_extraFlags} -fsanitize=address-full") # EXPENSIVE!
		else()
			set(_extraFlags "${_extraFlags} -fsanitize=address")
		endif()
		# Add ASAN_OPTIONS="detect_leaks=1" to cmdline to track leaks! - 2013.10.16
	endif ()
	if (CLANG_TSAN)
		set(_extraFlags "${_extraFlags} -fsanitize=thread")
	endif ()
	if (CLANG_MSAN)
		set(_extraFlags "${_extraFlags} -fsanitize=memory -fsanitize-memory-track-origins")
	endif ()
	if (CLANG_USAN)
		set(_extraFlags "${_extraFlags} -fsanitize=undefined")
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
		set(_extraFlags "${_extraFlags} -Wno-deprecated-register") # Qt code in travis-ci nodes is lame.
		set(_extraFlags "${_extraFlags} -Wno-documentation") # boost doxygen documentation is lame.
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

include_directories(support/include) # For routing access to ssu host and link_receiver
include_directories(krypto/include) # For cryptographic primitives
include_directories(ssu/include) # For routing access to ssu host and link_receiver
include_directories(routing/include) # For ssu access to routing client

add_subdirectory(3rdparty/plist)
add_subdirectory(support)
add_subdirectory(krypto)

add_subdirectory(ssu)
add_subdirectory(routing)
#add_subdirectory(naming)
add_subdirectory(nat)
add_subdirectory(ui)

add_subdirectory(playground)
