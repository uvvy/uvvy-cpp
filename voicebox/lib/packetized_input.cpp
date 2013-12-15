#include <mutex>
#include "packetized_input.h"

void packetized_input::set_frame_size(int framesize)
{
    super::set_frame_size(framesize);
    reset();
}

byte_array packetized_input::read_frame()
{
    // std::lock_guard<std::mutex> guard(mutex_);

    if (in_queue_.empty()) {
        return byte_array();
    }

    return in_queue_.dequeue();
}

void packetized_input::reset()
{
    disable();

    // std::lock_guard<std::mutex> guard(mutex_);
    in_queue_.clear();
}
