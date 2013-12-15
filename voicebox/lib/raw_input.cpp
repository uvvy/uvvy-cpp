#include "raw_input.h"

using namespace std;

raw_input::raw_input()
    : super()
{
    // XXX
    set_frame_size(960);
    set_sample_rate(48000.0);
}

void raw_input::accept_input(byte_array samplebuf)
{
    // Trivial XDR-based encoding, for debugging.
    byte_array bytebuf;
    // vector<float> 
    // SST::XdrStream xws(&bytebuf, QIODevice::WriteOnly);
    // for (unsigned int i = 0; i < frameSize(); i++)
    //     xws << samplebuf[i];

    // Queue it to the main thread
    in_queue_.enqueue(bytebuf);
}
