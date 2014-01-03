//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "opaque_endian.h"
#include "voicebox/jitterbuffer.h"
#include "voicebox/audio_service.h" // TRACE_DETAIL

/// @todo
/// [ ] Run set_enabled() all the way from the left side of pipeline to the right
/// This way jitterbuffer may delay enabling subsequent nodes until queue_min frames
/// has been received.
/// This means sinks also need to have a rightward pointer.
/// The only reason sinks work to the left is because rtaudio_sink is driven by the hw.
/// In all other cases some kind of cross-thread signal may be used to drive the whole chain.

using namespace std;

namespace voicebox {

/**
 * Maximum number of consecutive frames to skip
 */
static const int max_skip = 3;

/**
 * Minimum number of frames to queue before enabling output
 */
static const int queue_min = 10; // 1/5 sec  

// ^-- these are hardcoded for 50 packets per second (20ms packets)
// use min/max buffer latency in milliseconds instead.

/**
 * Maximum number of frames to queue before dropping frames
 */
static const int queue_max = 25; // 1/2 sec

/**
 * Called with mutex locked.
 */
void jitterbuffer::enqueue(byte_array data)
{
    bool wasempty = queue_.empty();
    queue_.emplace_back(data);
    // guard.unlock(); -- behavior change: on_ready_read() called with mutex held XXX @fixme

    // Notify reader if appropriate
    if (wasempty) {
        on_ready_read();
    }
}

//
// Packet acceptance into the jitterbuffer:
// If the packet is older than what is currently playing, simply drop it.
// If packet should be somewhere in the middle replacing an empty packet, replace.
// Otherwise check if there's a gap between the last packet in the queue and newly inserted one,
// if yes, insert a number of intermediate empty "missed" packets and then the new one.
// If queue size is too big, drop some of the older packets, playing catch up with the
// remote stream.
//
// Packet with the least timestamp is taken from the queue when produce_output() is called.
// The latest timestamp is then bumped to timestamp of that packet.
//
// Packets format:
// [8 bytes] microseconds since epoch (Jan 1, 2010)
// [4 bytes] sequence number
// [variable] payload
//
void jitterbuffer::accept_input(byte_array msg)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
#endif
    // log_packet_delay(msg);

    uint32_t seq_no = msg.as<big_uint32_t>()[2];
    uint32_t queue_first_seq_no = 0;

    unique_lock<mutex> guard(mutex_); // Need to lock the whole step here

    if (!queue_.empty()) {
        queue_first_seq_no = queue_.front().as<big_uint32_t>()[2];
    }

    int seqdiff = seq_no - sequence_number_;

    // If packet is out-of-order, see if we still can insert it somewhere.
    if (seqdiff < 0)
    {
        if (seq_no > queue_first_seq_no)
        {
            enqueue(msg);
            logger::debug(TRACE_RARE) << "Jitterbuffer - frame received out of order";

            // if there's a packet with the same sequence number in the queue - replace it
        }
        else // @todo: If the seq_no is directly before queue_first_seq_no or fits before with a gap
             // that's still within allowed queue size, prepend it to the JB queue!
        {
            logger::warning() << "Out-of-order packet too late - not fitting in JB; seq "
                << seq_no << ", expected seq " << sequence_number_;
        }
    }

    seqdiff = min(seqdiff, max_skip);
    // @todo When receiving too large a seqdiff, probably set up a full JB reset with
    // clearing the queue?

    // Queue up the missed frames, if any.
    for (int i = 0; i < seqdiff; ++i)
    {
        byte_array dummy;
        dummy.resize(12);
        enqueue(dummy);
        logger::debug(TRACE_RARE) << "MISSED audio frame " << sequence_number_ + i;
    }

    enqueue(msg);
#if REALTIME_CRIME
    logger::debug(TRACE_DETAIL) << "Received audio frame " << dec << seq_no;
#endif

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

    // Discard frames from the head if we exceed max queue size
    while (queue_.size() > queue_max) {
        queue_.pop_front();
    }
    bool emptied = queue_.empty();

    // Kick off further processing.
    // if (queue_.size() > queue_min) {
    //     acceptor->set_enabled(true);
    // }

    guard.unlock();

    // Remember which sequence we expect next
    sequence_number_ = seq_no + 1;

    if (emptied) {
        on_queue_empty();
    }
}

// Packets format:
// [8 bytes] microseconds since epoch (Jan 1, 2010)
// [4 bytes] sequence number
// [variable] payload
void jitterbuffer::produce_output(byte_array& buffer)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << __PRETTY_FUNCTION__;
#endif
    unique_lock<mutex> guard(mutex_);
    if (queue_.empty()) {
        guard.unlock();
        buffer.resize(0);
        return;
    }
    buffer = queue_.front();
    queue_.pop_front();
    bool emptied = queue_.empty();
    guard.unlock();

    if (emptied) {
        on_queue_empty();
    }
}

} // voicebox namespace

