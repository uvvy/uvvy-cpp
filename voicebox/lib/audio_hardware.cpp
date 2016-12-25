//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <mutex>
#include <set>
#include "RtAudio.h"
#include "voicebox/audio_hardware.h"
#include "voicebox/audio_sink.h"
#include "voicebox/audio_source.h"
#include "voicebox/audio_service.h"

using namespace std;
using namespace sss;

namespace voicebox {

static int num_devices = -1;
static int input_device = -1;
static int output_device = -1;

static int input_channels = 0;
static int output_channels = 0;

static std::mutex stream_mutex_;

static set<voicebox::audio_source*> instreams;
static set<voicebox::audio_sink*> outstreams;

static double hwrate;
static unsigned int hwframesize;

static int input_level, output_level;

audio_hardware* audio_hardware::instance()
{
    static audio_hardware hw;
    return &hw;
}

audio_hardware::audio_hardware()
{
    scan();
}

audio_hardware::~audio_hardware()
{
    close_audio();
    delete audio_inst; audio_inst = nullptr;
}

int audio_hardware::scan()
{
    if (num_devices >= 0) {
        return num_devices;
    }

    try {
        audio_inst = new RtAudio();
    }
    catch (RtError& error) {
        BOOST_LOG_TRIVIAL(warning) << "Can't initialize RtAudio library, " << error.what();
        return -1;
    }

    num_devices = audio_inst->getDeviceCount();
    if (num_devices == 0) {
        BOOST_LOG_TRIVIAL(warning) << "No audio devices available";
    }

    input_device = audio_inst->getDefaultInputDevice();
    output_device = audio_inst->getDefaultOutputDevice();

    return num_devices;
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

void audio_hardware::reopen()
{
    instance()->close_audio();
    instance()->open_audio();
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

    if (inputBuffer) {
        instance->capture(inputBuffer, nFrames);
    }
    if (outputBuffer) {
        instance->playback(outputBuffer, nFrames);
    }

    return 0;
}

int audio_hardware::get_sample_rate()
{
    return hwrate;
}

int audio_hardware::get_frame_size()
{
    return hwframesize;
}

void audio_hardware::set_input_level(int level)
{
    input_level = level;
}

void audio_hardware::set_output_level(int level)
{
    output_level = level;
}

// This simply computes a peak level of the signal, not signal energy.
static int compute_level(const float *buf, int nframes)
{
    float level{0.0};
    for (int i = 0; i < nframes; i++)
    {
        level = max(level, fabs(buf[i]));
    }
    if (level > 1.0)
    {
        BOOST_LOG_TRIVIAL(warning) << "TOO HIGH SIGNAL LEVEL " << level;
        // Needs compression...
    }

    return (int)(level * 100.0);
}

// Push to all registered sources in instreams.
void audio_hardware::capture(void* inputBuffer, unsigned int nFrames)
{
    lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we send

    nFrames *= input_channels;

    // Compute the audio input level if necessary
    // if (receivers(SIGNAL(inputLevelChanged(int))) > 0)
        set_input_level(compute_level(static_cast<float*>(inputBuffer), nFrames));

#if REALTIME_CRIME
    logger::debug(TRACE_DETAIL) << "Capturing to " << instreams.size() << " chains";
#endif

    // Broadcast the audio input to all listening input streams
    for (auto s : instreams)
    {
        if (s->sample_rate() == hwrate and s->frame_size() == hwframesize)
        {
            // The easy case - no buffering or resampling needed.  v--frame_bytes(nFrames)?
            s->accept_input(byte_array::wrap( //                   v
                static_cast<const char*>(inputBuffer), nFrames*sizeof(float)));
            continue;
        }

        // @todo Implement more tricky cases here, if necessary.
        assert(false);
    }
}

// Pull from all registered sinks in outstreams.
void audio_hardware::playback(void* outputBuffer, unsigned int nFrames)
{
    lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we mix

    nFrames *= output_channels;

    float* floatBuf = static_cast<float*>(outputBuffer);
    byte_array outbuf;

    // Simple cases: no streams mixing, set output level to 0,
    if (outstreams.empty())
    {
        fill_n(floatBuf, nFrames, 0.0);
        set_output_level(0);
        return;
    }

#if REALTIME_CRIME
    logger::debug(TRACE_DETAIL) << "Playing to " << outstreams.size() << " chains";
#endif

    // 1 stream is mixing, just produce_output() into target buffer.
    if (outstreams.size() == 1) {
        for (auto s : outstreams) {
            s->produce_output(outbuf);
        }
        copy_n(outbuf.as<float>(), nFrames, floatBuf);
    }
    else
    {
        // More than 1 stream mixing: mix into temp buffers then add and normalize.
        byte_array encbuf;
        fill_n(floatBuf, nFrames, 0.0);
        for (auto s : outstreams)
        {
            s->produce_output(encbuf);
            // Mixing in based on
            // http://dsp.stackexchange.com/questions/3581/algorithms-to-mix-audio-signals
            // to maintain the signal level of the mix.
            // Need to apply compressor/limiter afterwards if there are spikes.
            for (unsigned int j = 0; j < nFrames; ++j) {
                floatBuf[j] += encbuf.as<float>()[j];
            }
        }
    }

    // Compute the output level if necessary
    // if (receivers(SIGNAL(outputLevelChanged(int))) > 0)
        set_output_level(compute_level(floatBuf, nFrames));
}

void audio_hardware::open_audio()
{
    if (is_running()) {
        return; // Already open
    }

    // Make sure we're initialized
    // scan();

    lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we open

    bool enable_capture = !instreams.empty();
    bool enable_playback = !outstreams.empty();

    // See if any input or output streams are actually enabled
    if (!enable_capture and !enable_playback) {
        return;
    }

    // Use the maximum rate requested by any of our streams,
    // and the minimum framesize requested by any of our streams,
    // to maximize quality and minimize buffering latency.

    double max_rate{0.0};
    unsigned int min_frame_size{9999999};
    unsigned int max_in_channels{0};
    unsigned int max_out_channels{0};

    for (auto s : instreams)
    {
        max_rate = max(max_rate, s->sample_rate());
        min_frame_size = min(min_frame_size, s->frame_size());
        max_in_channels = max(max_in_channels, s->num_channels());
    }

    for (auto s : outstreams)
    {
        max_rate = max(max_rate, s->sample_rate());
        min_frame_size = min(min_frame_size, s->frame_size());
        max_out_channels = max(max_out_channels, s->num_channels());
    }

    // @todo Check against rates supported by devices, resample if necessary...

    // Open the audio device
    RtAudio::StreamParameters inparam, outparam;
    inparam.deviceId = input_device;
    inparam.nChannels = max_in_channels;
    outparam.deviceId = output_device;
    outparam.nChannels = max_out_channels;

    logger::debug(TRACE_DETAIL) << "Open audio: rate " << max_rate << ", frame " << min_frame_size
        << (enable_capture ? ", " : ", NO ") << "CAPTURE, device "
        << input_device << ", channels " << max_in_channels
        << (enable_playback ? "; " : "; NO ") << "PLAYBACK, device "
        << output_device << ", channels " << max_out_channels;

    try {
        audio_inst->openStream(enable_playback ? &outparam : NULL,
                               enable_capture ? &inparam : NULL,
                               RTAUDIO_FLOAT32, max_rate, &min_frame_size,
                               rtcallback, this);

        hwrate = max_rate;
        hwframesize = min_frame_size;
    }
    catch (RtError &error) {
        BOOST_LOG_TRIVIAL(warning) << "Couldn't open audio stream, " << error.what();
        throw;
    }

    input_channels = max_in_channels;
    output_channels = max_out_channels;

    start_audio();
}

void audio_hardware::close_audio()
{
    if (!is_running()) {
        return;
    }

    logger::debug(TRACE_ENTRY) << "Closing audio";

    try {
        lock_guard<mutex> guard(stream_mutex_); // Don't let fiddle with streams while we close
        audio_inst->closeStream();
    }
    catch (RtError &error) {
        BOOST_LOG_TRIVIAL(warning) << "Couldn't close audio stream, " << error.what();
        throw;
    }

    set_input_level(0);
    set_output_level(0);
}

void audio_hardware::start_audio()
{
    logger::debug(TRACE_ENTRY) << "Starting audio";
    audio_inst->startStream();
}

void audio_hardware::stop_audio()
{
    logger::debug(TRACE_ENTRY) << "Stopping audio";
    audio_inst->stopStream();
}

bool audio_hardware::is_running() const
{
    return audio_inst->isStreamOpen();
}

} // voicebox namespace
