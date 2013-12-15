
add_library(voicebox STATIC
    audio_service.cpp
    audio_sink.cpp
    audio_source.cpp

    audio_hardware.cpp
    audio_stream.cpp
    abstract_audio_input.cpp
    abstract_audio_output.cpp
    audio_sender.cpp
    audio_receiver.cpp
    audio_input.cpp
    audio_output.cpp
    audio_loopback.cpp
    file_looped_output.cpp
    opus_input.cpp
    opus_output.cpp
    packetized_input.cpp
    packetized_output.cpp
    raw_input.cpp
    raw_output.cpp)
