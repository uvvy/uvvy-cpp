include_directories(.)
include_directories(../3rdparty/opus-1.0.1/include ../3rdparty/rtaudio-4.0.11)

add_library(voiceclient STATIC
    audio_service.cpp
    audio_stream.cpp
    abstract_audio_input.cpp
    abstract_audio_output.cpp)
