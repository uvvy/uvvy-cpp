#include <boost/thread/locks.hpp> // lock_guard
#include "synchronized_queue.h"

using namespace std;

size_t synchronized_queue::size() const
{
    lock_guard<mutex> guard(mutex_);
    return queue_.size();
}

void synchronized_queue::clear()
{
    lock_guard<mutex> guard(mutex_);
    queue_.clear();
}

void synchronized_queue::enqueue(byte_array data)
{
    unique_lock<mutex> guard(mutex_);
    bool wasempty = queue_.empty();
    queue_.emplace_back(data);
    guard.unlock();

    // Notify reader if appropriate
    if (wasempty) {
        on_ready_read();
    }
}

byte_array synchronized_queue::dequeue()
{
    unique_lock<mutex> guard(mutex_);
        byte_array data = queue_.front();
        queue_.pop_front();
        emptied = queue_.empty();
    guard.unlock();

    if (emptied) {
        on_queue_empty();
    }
    return data;
}
