#include "jitterbuffer.h"

// Packet acceptance into the jitterbuffer:
// If the packet is older than what is currently playing, simply drop it.
// If packet should be somewhere in the middle replacing an empty packet, replace.
// Otherwise check if there's a gap between the last packet in the queue and newly inserted one,
// if yes, insert a number of intermediate empty "missed" packets and then the new one.
// If queue size is too big, drop some of the older packets, playing catch up with the
// remote stream.

// Packet with the least timestamp is taken from the queue when produce_output() is called.
// The latest timestamp is then bumbed to timestamp of that packet.

void jitterbuffer::accept_input(byte_array msg)
{
    log_packet_delay(msg);

    // Jitterbuffer part:
    queue_.enqueue(msg);

    sort(queue_.begin(), queue_.end(),
    [](byte_array const& a, byte_array const& b)
    {
        int64_t tsa = a.as<big_int64_t>()[0];
        int64_t tsb = b.as<big_int64_t>()[0];
        return tsa < tsb;
    });

    // Packets should be in strictly sequential order
    for (size_t i = 1; i < queue_.size(); ++i)
    {
        int64_t tsa = queue_[i-1].as<big_int64_t>()[0];
        int64_t tsb = queue_[i].as<big_int64_t>()[0];
        if (tsb - tsa > 10) {
            logger::warning(TRACE_DETAIL) << "Discontinuity in audio stream: packets are "
                << (tsb - tsa) << "ms apart.";
        }
    }
}

const int packetized_output::max_skip;

// // Determine how many frames we missed.
// int seqdiff = seq_no - out_sequence_;
// if (seqdiff < 0) {
//     // Out-of-order frame - just drop it for now.
//     // XXX insert into queue out-of-order if it's still useful
//     // -- basically should fit between (seqno-queue_max) .. seqno and be still in the queue...
//     // this implements a very basic jitterbuffer
//     logger::debug(199) << "packetized_output: frame received out of order";
//     return;
// }
// seqdiff = min(seqdiff, max_skip);

// // lock_guard<mutex> guard(mutex_); <- need to lock the whole step here

// // Queue up the missed frames, if any.
// for (int i = 0; i < seqdiff; i++) {
//     out_queue_.enqueue(byte_array());
//     logger::debug(199) << "MISSED audio frame " << out_sequence_ + i;
// }

// // Queue up the frame we actually got.
// out_queue_.enqueue(buf);
// logger::debug(200) << "Received audio frame" << seq_no;

// // Discard frames from the head if we exceed queueMax
// while (out_queue_.size() > queue_max) {
//     out_queue_.dequeue();
// }

// // Remember which sequence we expect next
// out_sequence_ = seq_no + 1;
