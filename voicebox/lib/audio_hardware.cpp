//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <mutex>
#include "RtAudio.h"
#include "audio_hardware.h"
#include "audio_sink.h"
#include "audio_source.h"

using namespace std;
using namespace ssu;

static std::mutex stream_mutex_;

static set<voicebox::audio_source*> instreams;
static set<voicebox::audio_sink*> outstreams;

static double hwrate;
static int hwframesize;

// @fixme Make a proper member
static int max_capture_channels() { return 1; }
static int max_playback_channels() { return 1; }

audio_hardware::audio_hardware()
{
    try {
        audio_inst  = new RtAudio();
    }
    catch (RtError& error) {
        logger::warning() << "Can't initialize RtAudio library, " << error.what();
        return;
    }

    open_audio();
}

audio_hardware::~audio_hardware()
{
    close_audio();
    delete audio_inst; audio_inst = 0;
}

bool audio_hardware::add_instream(voicebox::audio_source* in)
{
    lock_guard<mutex> guard(stream_mutex_);
    assert(!contains(instreams, in));
    bool wasempty = instreams.empty();
    instreams.insert(in);
    return wasempty;
}

bool audio_hardware::remove_instream(voicebox::audio_source* in)
{
    lock_guard<mutex> guard(stream_mutex_);
    assert(contains(instreams, in));
    instreams.erase(in);
    return instreams.empty();
}

bool audio_hardware::add_outstream(voicebox::audio_sink* out)
{
    lock_guard<mutex> guard(stream_mutex_);
    assert(!contains(outstreams, out));
    bool wasempty = outstreams.empty();
    outstreams.insert(out);
    return wasempty;
}

bool audio_hardware::remove_outstream(voicebox::audio_sink* out)
{
    lock_guard<mutex> guard(stream_mutex_);
    assert(contains(outstreams, out));
    outstreams.erase(out);
    return outstreams.empty();
}

static int rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
    double, RtAudioStreamStatus, void *userdata)
{
    audio_hardware* instance = reinterpret_cast<audio_hardware*>(userdata);

#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "rtcallback["<<instance<<"] outputBuffer " << outputBuffer
        << ", inputBuffer " << inputBuffer << ", nframes " << nFrames;
#endif

    // An RtAudio "frame" is one sample per channel,
    // whereas our "frame" is one buffer worth of data (as in Speex).

#if 0
    if (inputBuffer && outputBuffer) {
        copy_n((char*)inputBuffer, nFrames*sizeof(float), (char*)outputBuffer);
    }
#else
    if (inputBuffer) {
        instance->capture(inputBuffer, nFrames);
    }
    if (outputBuffer) {
        instance->playback(outputBuffer, nFrames);
    }
#endif
    return 0;
}

// Push to all registered sources in instreams.
void audio_hardware::capture(void* inputBuffer, unsigned int nFrames)
{
    lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we send

    // Broadcast the audio input to all listening input streams
    for (auto s : instreams)
    {
        if (s->sample_rate() == hwrate and s->frame_size() == hwframesize)
        {
            // The easy case - no buffering or resampling needed.  v--frame_bytes(nFrames)?
            s->accept_input(byte_array::wrap(
                static_cast<const char*>(inputBuffer), nFrames*sizeof(float)));
            continue;
        }

        // @todo Implement more tricky cases here, if necessary.
        assert(false);
    }

    // Now can remove that sender dude..
    // sender_->send_packet((float*)inputBuffer, nFrames);
}

// Pull from all registered sinks in outstreams.
void audio_hardware::playback(void* outputBuffer, unsigned int nFrames)
{
    lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we mix
    receiver_->get_packet((float*)outputBuffer, nFrames);

    // Simple cases: no streams mixing, set output level to 0,
    // 1 stream is mixing, just produce_output() into target buffer.
    // More than 1 stream mixing: mix into temp buffers than add and normalize.
}

#define SAMPLE_RATE 48000

void audio_hardware::open_audio()
{
    // Open the audio device
    RtAudio::StreamParameters inparam, outparam;
    inparam.deviceId = audio_inst->getDefaultInputDevice();
    inparam.nChannels = max_capture_channels(); // of all instreams
    outparam.deviceId = audio_inst->getDefaultOutputDevice();
    outparam.nChannels = max_playback_channels(); // of all outstreams
    unsigned int bufferFrames = SAMPLE_RATE/100; // 10ms

    try {
        audio_inst->openStream(&outparam, &inparam, RTAUDIO_FLOAT32, SAMPLE_RATE, &bufferFrames,
            rtcallback, this);

        hwrate = SAMPLE_RATE;
        hwframesize = bufferFrames;
    }
    catch (RtError &error) {
        logger::warning() << "Couldn't open audio stream, " << error.what();
        throw;
    }
}

void audio_hardware::close_audio()
{
    try {
        lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we down
        audio_inst->closeStream();
    }
    catch (RtError &error) {
        logger::warning() << "Couldn't close audio stream, " << error.what();
        throw;
    }
}

void audio_hardware::start_audio()
{
    audio_inst->startStream();
}

void audio_hardware::stop_audio()
{
    audio_inst->stopStream();
}
