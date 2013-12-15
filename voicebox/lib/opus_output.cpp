#include "logging.h"
#include "opus_output.h"

using namespace std;

void opus_output::set_enabled(bool enabling)
{
    if (enabling and !is_enabled())
    {
        assert(!decstate);
        int error = 0;
        decstate = opus_decoder_create(48000, num_channels(), &error);
        assert(decstate);
        assert(!error);

        int framesize, rate;
        opus_decoder_ctl(decstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms
        set_frame_size(framesize);
        set_sample_rate(rate);
        logger::debug() << "opus_output: frame size " << framesize << " sample rate " << rate;

        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);

        assert(decstate);
        opus_decoder_destroy(decstate);
        decstate = NULL;
    }
}

void opus_output::produce_output(float *samplebuf)
{
    // Grab the next buffer from the queue
    byte_array bytebuf;
    if (!out_queue_.empty()) { // @todo May become empty here...
        bytebuf = out_queue_.dequeue();
    }

    // Decode the frame
    if (!bytebuf.is_empty())
    {
        logger::debug() << "Decode frame size: " << bytebuf.size();
        int len = opus_decode_float(decstate, (unsigned char*)bytebuf.data(),
            bytebuf.size(), samplebuf, frame_size(), /*decodeFEC:*/0);
        // assert(len > 0);
        assert(len == frame_size());
    }
    else
    {
        // "decode" a missing frame
        int len = opus_decode_float(decstate, NULL, 0, samplebuf, frame_size(), /*decodeFEC:*/0);
        assert(len > 0);
    }
}
