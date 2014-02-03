cmake_minimum_required(VERSION 2.8)
project(metakit)

set(CMAKE_OSX_ARCHITECTURES x86_64)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckTypeSize)

check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(memory.h HAVE_MEMORY_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)

check_function_exists(bcopy HAVE_BCOPY)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(mmap HAVE_MMAP)

set(CMAKE_EXTRA_INCLUDE_FILES)
check_type_size(long SIZEOF_LONG)
check_type_size("long long" LONG_LONG)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(demos)
add_subdirectory(tests)
