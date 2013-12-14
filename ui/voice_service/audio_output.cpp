#include "audio_output.h"

using namespace std;

int audio_output::write_frames(const float *buf, int nframes)
{
    int framesize = frame_size();
    lock_guard<mutex> guard(mutex_);
    for (int i = 0; i < nframes; i++) {
        byte_array frame(framesize * sizeof(float));
        memcpy(frame.data(), buf, framesize * sizeof(float));
        out_queue_.push_back(frame);
        buf += framesize;
    }
    return nframes;
}

int audio_output::write_frames(const std::vector<float> &buf)
{
    assert(buf.size() % frame_size() == 0);
    int nframes = buf.size() / frame_size();
    return write_frames(&buf[0], nframes);
}

void audio_output::produce_output(float *buf)
{
    int framesize = frame_size();

    unique_lock<mutex> guard(mutex_);
    bool emptied{false};

    if (out_queue_.empty())
    {
        memset(buf, 0, framesize * sizeof(float));
        emptied = false;
    }
    else
    {
        byte_array frame = out_queue_.front();
        out_queue_.pop_front();
        memcpy(buf, frame.data(), framesize * sizeof(float));
        emptied = out_queue_.empty();
    }
    guard.unlock();

    if (emptied) {
        on_queue_empty();
    }
}
