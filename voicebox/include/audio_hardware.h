//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <memory>
#include <functional>
#include "server.h"
#include "stream.h"

class RtAudio;

namespace voicebox {

class audio_sink;
class audio_source;

/**
 * Controls the audio layer and starts playback and capture.
 */
class audio_hardware
{
    RtAudio* audio_inst{0};

    audio_hardware();
    ~audio_hardware();

public:
    static audio_hardware* instance();

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

    void set_input_level(int level);
    void set_output_level(int level);

    // Hardware I/O handlers called from rtcallback
    void capture(void* buffer, unsigned int nFrames);
    void playback(void* buffer, unsigned int nFrames);
};

} // voicebox namespace
