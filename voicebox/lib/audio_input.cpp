#include "audio_input.h"

using namespace std;

audio_input::~audio_input()
{
    reset();
}

int audio_input::read_frames_available()
{
    return in_queue_.size();
}

int audio_input::read_frames(float *buf, int maxframes)
{
    assert(maxframes >= 0);

    // lock_guard<mutex> guard(mutex_);
    int nframes = min(maxframes, (int)in_queue_.size());
    read_into(buf, nframes);
    return nframes;
}

std::vector<float> audio_input::read_frames(int maxframes)
{
    assert(maxframes >= 0);

    // lock_guard<mutex> guard(mutex_);
    int nframes = min(maxframes, (int)in_queue_.size());
    std::vector<float> ret;
    ret.resize(nframes * frame_size());
    read_into(ret.data(), nframes);
    return ret;
}

/**
 * Must be called with mutex held.
 */
void audio_input::read_into(float *buf, int nframes)
{
    for (int i = 0; i < nframes; i++)
    {
        byte_array frame = in_queue_.dequeue();
        memcpy(buf, frame.data(), frame_bytes());
        buf += frame_size();
    }
}

void audio_input::accept_input(byte_array frame)
{
    // Queue the buffer
    in_queue_.enqueue(frame);
}
