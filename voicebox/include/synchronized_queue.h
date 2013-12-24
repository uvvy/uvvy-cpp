//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <deque>
#include <mutex>
#include <boost/signals2/signal.hpp>

template <typename T>
class synchronized_queue
{
    // Inter-thread synchronization and queueing state
    mutable std::mutex mutex_;     // Protection for input queue
    std::deque<T> queue_; // Queue of audio input frames

public:
    synchronized_queue() = default;

    void clear()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        queue_.clear();
    }

    inline bool empty() const { return size() == 0; }

    size_t size() const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return queue_.size();
    }

    void enqueue(T data);
    T dequeue();

    typedef boost::signals2::signal<void (void)> state_signal;
    state_signal on_ready_read;
    state_signal on_queue_empty;
};

template <typename T>
void synchronized_queue<T>::enqueue(T data)
{
    std::unique_lock<std::mutex> guard(mutex_);
    bool wasempty = queue_.empty();
    queue_.emplace_back(data);
    guard.unlock();

    // Notify reader if appropriate
    if (wasempty) {
        on_ready_read();
    }
}

template <typename T>
T synchronized_queue<T>::dequeue()
{
    std::unique_lock<std::mutex> guard(mutex_);
    T data = queue_.front();
    queue_.pop_front();
    bool emptied = queue_.empty();
    guard.unlock();

    if (emptied) {
        on_queue_empty();
    }
    return data;
}
