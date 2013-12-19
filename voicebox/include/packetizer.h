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
 * It doesn't have, neither use acceptor or producer, since it sits at the intersection
 * of push and pull threads, all other components call into packetizer.
 * It passes the data on, using synchronization.
 * The actual packetization happens in the producer for this node, this class only
 * accepts fixed-size packets.
 *
 * @todo
 * Packetizer needs a side channel to record and transport timestamp information
 * about every packet.
 * Encoder sinks would embed these timestamps into the final buffer.
 * Decoder sinks would usually ignore it, since on the receiving side the jitterbuffer uses this
 * timestamp info.
 */
class packetizer : public audio_source, public audio_sink
{
protected:
    synchronized_queue queue_;

public:
    packetizer() = default;

    void produce_output(byte_array& buffer) override; // from sink
    void accept_input(byte_array data) override; // from source
};

}
