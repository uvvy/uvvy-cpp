set(VOICEBOX_LIBS voicebox rtaudio)

if (APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(COREFOUNDATION_LIB CoreFoundation)
    list(APPEND VOICEBOX_LIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
endif (APPLE)

if (UNIX AND NOT APPLE)
    find_library(PULSE_LIB pulse)
    find_library(PULSESIMPLE_LIB pulse-simple)
    list(APPEND VOICEBOX_LIBS ${PULSE_LIB} ${PULSESIMPLE_LIB})
endif (UNIX AND NOT APPLE)

include_directories(include)
include_directories(../ui/3rdparty/opus-1.0.1/include ../ui/3rdparty/rtaudio)

add_subdirectory(lib)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif (BUILD_TESTING)

# TODO: This breaks modularity...
set(VOICEBOX_LIBS ${VOICEBOX_LIBS} PARENT_SCOPE)
