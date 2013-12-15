#include "logging.h"
#include "opus_encode_sink.h"

using namespace std;

namespace voicebox {

void opus_encode_sink::set_enabled(bool enabling)
{
    if (enabling and !is_enabled()) {
        assert(!encstate);
        int error = 0;
        encstate = opus_encoder_create(sample_rate(), num_channels(),
            OPUS_APPLICATION_VOIP, &error);
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

void opus_encode_sink::produce_output(byte_array& buffer)
{
    // Get data from our producer, if any.
    byte_array samplebuf;
    if (producer()) {
        producer()->produce_output(samplebuf);
    }

    // Encode the frame and write it into a buffer
    int maxbytes = 1024;//meh, any opus option to get this?
    buffer.resize(maxbytes);
    int nbytes = opus_encode_float(encstate, (const float*)samplebuf.data(), frame_size(),
        (unsigned char*)bytebuf.data(), bytebuf.size());
    assert(nbytes <= maxbytes);
    buffer.resize(nbytes);
    logger::debug() << "Encoded frame size: " << nbytes;
}

} // voicebox namespace
