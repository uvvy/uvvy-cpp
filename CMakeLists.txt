cmake_minimum_required(VERSION 2.8)

project(Uvvy)

include(CTest)
include(cmake/setup.cmake)

# Export compile commands for ST3 clang plugin.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set flag for submodules build
set(BUILD_IN_UVVY ON)

if (APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif (APPLE)

include_directories(3rdparty/endian) # Additional unofficial boost libraries
include_directories(3rdparty/sodiumpp/sodiumpp/include) # sodiumpp

include_directories(arsenal/include) # For routing access to sss host and link_receiver
include_directories(krypto/include) # For cryptographic primitives
include_directories(uia/include)
#include_directories(sss/include) # For routing access to sss host and link_receiver
#include_directories(routing/include) # For sss access to routing client
#include_directories(voicebox/include)

# Cryptographic support
if (BUILD_TESTING)
    set(SODIUMPP_TEST 1)
endif()
add_subdirectory(3rdparty/sodiumpp)
#add_subdirectory(krypto)
# Various C++ utilities
add_subdirectory(arsenal)
# Minimal socket and channel functionality. Base substrate.
add_subdirectory(uia)
# Peer-to-peer overlay network and DHT
#add_subdirectory(routing)
# Name resolution overlay network
#add_subdirectory(naming)
# NAT passthrough tools
#add_subdirectory(nat)
# Streams and stream-oriented communication. High level abstractions.
#add_subdirectory(sss)
# 3rd-party libraries
#add_subdirectory(3rdparty/opus)
#add_subdirectory(3rdparty/rtaudio)
#add_subdirectory(3rdparty/metakit)
# Audio pipeline
#add_subdirectory(voicebox)
# File synchronisation
#add_subdirectory(filesyncbox)

#add_subdirectory(gui)

#add_subdirectory(demos)
#add_subdirectory(playground)
