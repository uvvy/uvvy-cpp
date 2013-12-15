#include "logging.h"
#include "packetized_output.h"

using namespace std;

const int packetized_output::max_skip;

packetized_output::~packetized_output()
{
    reset();
}

void packetized_output::set_frame_size(int framesize)
{
    super::set_frame_size(framesize);
    reset();
}

void packetized_output::write_frame(byte_array const& buf, uint64_t seq_no, size_t queue_max)
{
    // Determine how many frames we missed.
    int seqdiff = seq_no - out_sequence_;
    if (seqdiff < 0) {
        // Out-of-order frame - just drop it for now.
        // XXX insert into queue out-of-order if it's still useful
        // -- basically should fit between (seqno-queue_max) .. seqno and be still in the queue...
        // this implements a very basic jitterbuffer
        logger::debug(199) << "packetized_output: frame received out of order";
        return;
    }
    seqdiff = min(seqdiff, max_skip);

    // lock_guard<mutex> guard(mutex_);

    // Queue up the missed frames, if any.
    for (int i = 0; i < seqdiff; i++) {
        out_queue_.enqueue(byte_array());
        logger::debug(199) << "MISSED audio frame " << out_sequence_ + i;
    }

    // Queue up the frame we actually got.
    out_queue_.enqueue(buf);
    logger::debug(200) << "Received audio frame" << seq_no;

    // Discard frames from the head if we exceed queueMax
    while (out_queue_.size() > queue_max) {
        out_queue_.dequeue();
    }

    // Remember which sequence we expect next
    out_sequence_ = seq_no + 1;
}

size_t packetized_output::num_frames_queued()
{
    return out_queue_.size();
}

void packetized_output::reset()
{
    disable();

    // lock_guard<mutex> guard(mutex_);
    out_queue_.clear();
}

