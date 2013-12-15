#pragma once

#include "abstract_audio_input.h"
#include "synchronized_queue.h"

/**
 * This class represents a source of audio input,
 * providing automatic queueing and interthread synchronization.
 * It assumes the signal will be packetized before transporting.
 */
class packetized_input : public abstract_audio_input
{
    typedef abstract_audio_input super;

protected:
    synchronized_queue in_queue_;

public:
    packetized_input() = default;

    void set_frame_size(int framesize);

    byte_array read_frame();

    // Disable the input stream and clear the input queue.
    void reset();

    typedef boost::signals2::signal<void (void)> ready_signal;
    ready_signal on_ready_read;
};
