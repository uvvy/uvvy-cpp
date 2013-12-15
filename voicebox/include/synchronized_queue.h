#pragma once

#include <deque>
#include <mutex>
#include <boost/signals2/signal.hpp>
#include "byte_array.h"

class synchronized_queue
{
    // Inter-thread synchronization and queueing state
    std::mutex mutex_;             // Protection for input queue
    std::deque<byte_array> queue_; // Queue of audio input frames

    typedef boost::signals2::signal<void (void)> state_signal;
    state_signal on_ready_read;
    state_signal on_queue_empty;

public:
    synchronized_queue() = default;

    void clear();
    inline bool empty() const { return size() == 0; }
    size_t size() const;

    void enqueue(byte_array data);
    byte_array dequeue();
};
