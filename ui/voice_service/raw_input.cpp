#include "raw_input.h"

using namespace std;

raw_input::raw_input()
    : super()
{
    // XXX
    set_frame_size(960);
    set_sample_rate(48000.0);
}

void raw_input::accept_input(const float *samplebuf)
{
    // Trivial XDR-based encoding, for debugging.
    byte_array bytebuf;
    // SST::XdrStream xws(&bytebuf, QIODevice::WriteOnly);
    // for (unsigned int i = 0; i < frameSize(); i++)
    //     xws << samplebuf[i];

    // the second part can be a common helper functions it seems:

    // Queue it to the main thread
    unique_lock<mutex> guard(mutex_);
    bool wasempty = in_queue_.empty();
    in_queue_.push_back(bytebuf);
    guard.unlock();

    // Signal the main thread if appropriate
    if (wasempty) {
        on_ready_read();
    }
}
