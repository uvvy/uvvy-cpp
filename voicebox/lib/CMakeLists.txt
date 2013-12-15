
add_library(voicebox STATIC
    synchronized_queue.cpp
    audio_service.cpp
    audio_sink.cpp
    audio_source.cpp
    rtaudio_sink.cpp
    rtaudio_source.cpp
    audio_hardware.cpp
    audio_stream.cpp
    opus_encode_sink.cpp
    opus_decode_sink.cpp

    audio_sender.cpp
    audio_receiver.cpp)

#    audio_input.cpp
#    audio_output.cpp
#    audio_loopback.cpp
#    file_looped_output.cpp
#    packetized_input.cpp
#    packetized_output.cpp
#    raw_input.cpp
#    raw_output.cpp
