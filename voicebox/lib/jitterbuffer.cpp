#include "logging.h"
#include "jitterbuffer.h"
#include "opaque_endian.h"
#include "audio_service.h" // TRACE_DETAIL

using namespace std;

namespace voicebox {

/**
 * Maximum number of consecutive frames to skip
 */
static const int max_skip = 3;

jitterbuffer::jitterbuffer()
{
    queue_.on_ready_read.connect([this] { on_ready_read(); });
    queue_.on_queue_empty.connect([this] { on_queue_empty(); });
}

// Packet acceptance into the jitterbuffer:
// If the packet is older than what is currently playing, simply drop it.
// If packet should be somewhere in the middle replacing an empty packet, replace.
// Otherwise check if there's a gap between the last packet in the queue and newly inserted one,
// if yes, insert a number of intermediate empty "missed" packets and then the new one.
// If queue size is too big, drop some of the older packets, playing catch up with the
// remote stream.

// Packet with the least timestamp is taken from the queue when produce_output() is called.
// The latest timestamp is then bumped to timestamp of that packet.

// Packets format:
// [8 bytes] microseconds since epoch (Jan 1, 2010)
// [4 bytes] sequence number
// [variable] payload
// sequence number is coming from where? - see voice.cc:156
void jitterbuffer::accept_input(byte_array msg)
{
    // log_packet_delay(msg);

    uint32_t seq_no = msg.as<big_uint32_t>()[2];
    uint32_t queue_first_seq_no = queue_.front().as<big_uint32_t>()[2];

    int seqdiff = seq_no - sequence_number_;

    // If packet is out-of-order, see if we still can insert it somewhere.
    if (seqdiff < 0)
    {
        if (seq_no > queue_first_seq_no)
        {
            queue_.enqueue(msg);
            logger::debug(199) << "Jitterbuffer - frame received out of order";

            // if there's a packet with the same sequence number in the queue - replace it
        }
        else
        {
            logger::warning() << "Out-of-order packet too late - not fitting in JB";
        }
    }

    seqdiff = min(seqdiff, max_skip);
    // @todo When receiving too large a seqdiff, probably set up a full JB reset with
    // clearing the queue?

// lock_guard<mutex> guard(mutex_); <- need to lock the whole step here

    // Queue up the missed frames, if any.
    for (int i = 0; i < seqdiff; ++i)
    {
        queue_.enqueue(byte_array());
        logger::debug(199) << "MISSED audio frame " << sequence_number_ + i;
    }

    queue_.enqueue(msg);
    logger::debug(200) << "Received audio frame " << dec << seq_no;

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

// // Discard frames from the head if we exceed queueMaxSize
// while (queue_.size() > queue_max) {
//     queue_.dequeue();
// }

    // Remember which sequence we expect next
    sequence_number_ = seq_no + 1;
}

// Packets format:
// [8 bytes] microseconds since epoch (Jan 1, 2010)
// [4 bytes] sequence number
// [variable] payload
void jitterbuffer::produce_output(byte_array& buffer)
{}

} // voicebox namespace

