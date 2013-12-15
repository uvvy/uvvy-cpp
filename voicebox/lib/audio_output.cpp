#include "audio_output.h"

using namespace std;

// called by netlayer?
int audio_output::write_frames(const float *buf, int nframes)
{
    // lock_guard<mutex> guard(mutex_);
    for (int i = 0; i < nframes; i++) {
        byte_array frame(frame_bytes());
        memcpy(frame.data(), buf, frame_bytes());
        out_queue_.enqueue(frame);
        buf += frame_size();
    }
    return nframes;
}

int audio_output::write_frames(const std::vector<float> &buf)
{
    assert(buf.size() % frame_size() == 0);
    int nframes = buf.size() / frame_size();
    return write_frames(&buf[0], nframes);
}

// called from rtcallback
void audio_output::produce_output(float *buf)
{
    if (out_queue_.empty())
    {
        memset(buf, 0, frame_bytes());
    }
    else
    {
        byte_array frame = out_queue_.dequeue();
        memcpy(buf, frame.data(), frame_bytes());
    }
}
