//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

namespace voicebox {

/**
 * It doesn't have, neither use, acceptor or producer, since it sits at the intersection
 * of push and pull threads, all other components call into packetizer.
 * It passes the data on, using synchronization.
 * The actual packetization happens in the producer for this node, this class only
 * accepts fixed-size packets.
 *
 * Packetizer needs a side channel to record and transport timestamp information
 * about every packet. It is currently embedded directly into the produced buffer.
 * Encoder sinks would embed these timestamps into the final buffer.
 * Jitterbuffer would use this timestamp info on the receiving side.
 *
 * accept_input packet format:
 * [frame_bytes()] uncompressed audio data frame (framed by an audio_source usually)
 *
 * produce_output packet format:
 * [8 bytes] microseconds since epoch (Jan 1, 2000 0:00:00 UTC)
 * [frame_bytes()] uncompressed audio data frame
 *
 * Packetizer's on_ready_read() signal is often connected to the network sink send driver,
 * since presence of packets in the output queue is a good enough reason to send, most of the time.
 */
class packetizer : public audio_source, public audio_sink
{
protected:
    synchronized_queue queue_;

public:
    packetizer() = default;

    void produce_output(byte_array& buffer) override; // from sink
    void accept_input(byte_array data) override; // from source

    synchronized_queue::state_signal on_ready_read;
    synchronized_queue::state_signal on_queue_empty;
};

}
