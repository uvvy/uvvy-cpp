#include "audio_input.h"

using namespace std;

audio_input::~audio_input()
{
    reset();
}

int audio_input::read_frames_available()
{
    lock_guard<mutex> guard(mutex_);
    return in_queue_.size();
}

int audio_input::read_frames(float *buf, int maxframes)
{
    assert(maxframes >= 0);

    lock_guard<mutex> guard(mutex_);
    int nframes = min(maxframes, (int)in_queue_.size());
    read_into(buf, nframes);
    return nframes;
}

std::vector<float> audio_input::read_frames(int maxframes)
{
    assert(maxframes >= 0);

    lock_guard<mutex> guard(mutex_);
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
    int framesize = frame_size();
    for (int i = 0; i < nframes; i++) {
        byte_array frame = in_queue_.front();
        in_queue_.pop_front();
        memcpy(buf, frame.data(), framesize * sizeof(float));
        buf += framesize;
    }
}

void audio_input::accept_input(const float *buf)
{
    // Copy the input frame into a buffer for queueing
    int framesize = frame_size();
    byte_array frame(framesize * sizeof(float));
    memcpy(frame.data(), buf, framesize * sizeof(float));

    // Queue the buffer
    unique_lock<mutex> guard(mutex_);
    bool wasempty = in_queue_.empty();
    in_queue_.push_back(frame);
    guard.unlock();

    // Notify reader if appropriate
    if (wasempty) {
        on_ready_read();
    }
}
