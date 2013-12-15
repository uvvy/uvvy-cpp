#include "raw_output.h"

using namespace std;

raw_output::raw_output()
    : super()
{
    // XXX
    set_frame_size(960);
    set_sample_rate(48000.0);
}

void raw_output::produce_output(float *samplebuf)
{
    // Grab the next buffer from the queue
    byte_array bytebuf;
    if (!out_queue_.empty()) {
        bytebuf = out_queue_.dequeue();
    }

    // Trivial XDR-based encoding, for debugging
    if (!bytebuf.is_empty())
    {
        // SST::XdrStream xrs(&bytebuf, QIODevice::ReadOnly);
        // for (unsigned int i = 0; i < frameSize(); i++)
        //     xrs >> samplebuf[i];
    } else {
        memset(samplebuf, 0, frame_bytes());
    }
}

