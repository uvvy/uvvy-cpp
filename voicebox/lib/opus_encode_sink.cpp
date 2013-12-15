#include "logging.h"
#include "opus_input.h"

using namespace std;

void opus_input::set_enabled(bool enabling)
{
    if (enabling and !is_enabled()) {
        assert(!encstate);
        int error = 0;
        encstate = opus_encoder_create(48000, num_channels(), OPUS_APPLICATION_VOIP, &error);
        assert(encstate);
        assert(!error);

        int framesize, rate;
        opus_encoder_ctl(encstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms
        set_frame_size(framesize);
        set_sample_rate(rate);
        logger::debug() << "opus_input: frame size " << framesize << " sample rate " << rate;

        opus_encoder_ctl(encstate, OPUS_SET_VBR(1));
        opus_encoder_ctl(encstate, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encstate, OPUS_SET_DTX(1));

        super::set_enabled(true);

    } else if (!enabling and is_enabled()) {

        super::set_enabled(false);

        assert(encstate);
        opus_encoder_destroy(encstate);
        encstate = nullptr;
    }
}

void opus_input::accept_input(byte_array samplebuf)
{
    // Encode the frame and write it into a buffer
    byte_array bytebuf;
    int maxbytes = 1024;//meh, any opus option to get this?
    bytebuf.resize(maxbytes);
    int nbytes = opus_encode_float(encstate, (const float*)samplebuf.data(), frame_size(),
        (unsigned char*)bytebuf.data(), bytebuf.size());
    assert(nbytes <= maxbytes);
    bytebuf.resize(nbytes);
    logger::debug() << "Encoded frame size: " << nbytes;

    in_queue_.enqueue(bytebuf);
}

