#include "audio_hardware.h"
#include "RtAudio.h"

static std::set<abstract_audio_input*> instreams;
static std::set<abstract_audio_output*> outstreams;

audio_hardware::audio_hardware(audio_sender* sender, audio_receiver* receiver)
    : sender_(sender)
    , receiver_(receiver)
{
    try {
        audio_inst  = new RtAudio();
    }
    catch (RtError &error) {
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

bool audio_hardware::add_instream(abstract_audio_input* in)
{
    assert(!contains(instreams, in));
    bool wasempty = instreams.empty();
    instreams.insert(in);
    return wasempty;
}

bool audio_hardware::remove_instream(abstract_audio_input* in)
{
    assert(contains(instreams, in));
    instreams.erase(in);
    return instreams.empty();
}

bool audio_hardware::add_outstream(abstract_audio_output* out)
{
    assert(!contains(outstreams, out));
    bool wasempty = outstreams.empty();
    outstreams.append(out);
    return wasempty;
}

bool audio_hardware::remove_outstream(abstract_audio_output* out)
{
    assert(contains(outstreams, out));
    outstreams.erase(out);
    return outstreams.empty();
}

void audio_hardware::open_audio()
{
    // Open the audio device
    RtAudio::StreamParameters inparam, outparam;
    inparam.deviceId = audio_inst->getDefaultInputDevice();
    inparam.nChannels = audio_sender::nChannels;
    outparam.deviceId = audio_inst->getDefaultOutputDevice();
    outparam.nChannels = audio_receiver::nChannels;
    unsigned int bufferFrames = 480; // 10ms

    try {
        audio_inst->openStream(&outparam, &inparam, RTAUDIO_FLOAT32, 48000, &bufferFrames,
            rtcallback, this);
    }
    catch (RtError &error) {
        logger::warning() << "Couldn't open stream, " << error.what();
        throw;
    }
}

void audio_hardware::close_audio()
{
    try {
        audio_inst->closeStream();
    }
    catch (RtError &error) {
        logger::warning() << "Couldn't close stream, " << error.what();
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

void audio_hardware::new_connection(shared_ptr<server> server,
    std::function<void(void)> on_start,
    std::function<void(void)> on_stop)
{
    auto stream = server->accept();
    if (!stream)
        return;

    logger::info() << "New incoming connection from " << stream->remote_host_id();
    streaming(stream);

    stream->on_link_up.connect([this,on_start] {
        on_start();
        start_audio();
    });

    stream->on_link_down.connect([this,on_stop] {
        on_stop();
        stop_audio();
    });

    if (stream->is_link_up()) {
        logger::debug() << "Incoming stream is ready, start sending immediately.";
        on_start();
        start_audio();
    }
}

void audio_hardware::streaming(shared_ptr<stream> stream)
{
    sender_->streaming(stream);
    receiver_->streaming(stream);
}

static int audio_hardware::rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
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
        instance->sender_->send_packet((float*)inputBuffer, nFrames);
    }
    if (outputBuffer) {
        instance->receiver_->get_packet((float*)outputBuffer, nFrames);
    }
#endif
    return 0;
}
