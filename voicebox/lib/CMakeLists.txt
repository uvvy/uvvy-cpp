
add_library(voicebox STATIC
    jitterbuffer.cpp
    audio_hardware.cpp
    audio_stream.cpp
    audio_service.cpp
    audio_sink.cpp
    audio_source.cpp
    rtaudio_sink.cpp
    rtaudio_source.cpp
    opus_encode_sink.cpp
    opus_decode_sink.cpp
    file_read_sink.cpp
    packet_sink.cpp)
