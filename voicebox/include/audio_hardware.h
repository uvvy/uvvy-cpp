#pragma once

#include <memory>
#include <functional>
#include "server.h"
#include "stream.h"

class RtAudio;
class audio_sender; // @todo Remove
class audio_receiver; // @todo Remove
class abstract_audio_input;
class abstract_audio_output;

/**
 * Controls the audio layer and starts playback and capture.
 */
class audio_hardware
{
    RtAudio* audio_inst{0};
    audio_sender* sender_{0};
    audio_receiver* receiver_{0};

public:
    audio_hardware(audio_sender* sender, audio_receiver* receiver);
    ~audio_hardware();

    static bool add_instream(abstract_audio_input* in);
    static bool remove_instream(abstract_audio_input* in);
    static bool add_outstream(abstract_audio_output* out);
    static bool remove_outstream(abstract_audio_output* out);

    static void reopen();
    static int get_sample_rate();
    static int get_frame_size();

    void open_audio();
    void close_audio();

    void start_audio();
    void stop_audio();

    // @todo Move ssu-related handlers into a different class,
    // leave only audio-hardware-related things here.
    void new_connection(std::shared_ptr<ssu::server> server,
        std::function<void(void)> on_start,
        std::function<void(void)> on_stop);

    void streaming(std::shared_ptr<ssu::stream> stream);

    // Hardware I/O handlers called from rtcallback
    void capture(void* buffer, unsigned int nFrames);
    void playback(void* buffer, unsigned int nFrames);
};
