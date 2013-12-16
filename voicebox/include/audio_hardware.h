#pragma once

#include <memory>
#include <functional>
#include "server.h"
#include "stream.h"

class RtAudio;
namespace voicebox {
    class audio_sink;
    class audio_source;
} // voicebox namespace

/**
 * Controls the audio layer and starts playback and capture.
 */
class audio_hardware
{
    RtAudio* audio_inst{0};

public:
    audio_hardware();
    ~audio_hardware();

    static bool add_instream(voicebox::audio_source* in);
    static bool remove_instream(voicebox::audio_source* in);
    static bool add_outstream(voicebox::audio_sink* out);
    static bool remove_outstream(voicebox::audio_sink* out);

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
