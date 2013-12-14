#pragma once

#include <deque>
#include <mutex>
#include <boost/signals2/signal.hpp>
#include "byte_array.h"
#include "abstract_audio_output.h"

/**
 * This class represents a high-level sink for audio output
 * to the currently selected output device, at a controllable bitrate,
 * providing automatic queueing and interthread synchronization.
 * It assumes data arrives in packets with undetermined delay.
 * This leads to use of jitterbuffer (in the future) and other tricks.
 */
class packetized_output : public abstract_audio_output
{
    typedef abstract_audio_output super;

protected:
    /**
     * Maximum number of consecutive frames to skip
     */
    static const int max_skip = 3;

    // Inter-thread synchronization and queueing state
    std::mutex mutex_;                 // Protection for output queue
    std::deque<byte_array> out_queue_; // Queue of audio output frames
    uint64_t out_sequence_{0};

public:
    packetized_output() = default;
    packetized_output(int framesize, double samplerate, int channels = 1)
        : abstract_audio_output(framesize, samplerate, channels)
    {}
    ~packetized_output();

    void set_frame_size(int framesize);

    /**
     * Write a frame with the given seqno to the tail of the queue,
     * padding the queue as necessary to account for missed frames.
     * If the queue gets longer than queuemax, drop frames from the head.
     * @param buf      [description]
     * @param seqno    [description]
     * @param queuemax [description]
     */
    void write_frame(byte_array const& buf, uint64_t seq_no, size_t queue_max);

    /**
     * Return the current length of the output queue.
     */
    size_t num_frames_queued();

    /**
     * Disable the output stream and clear the output queue.
     */
    void reset();

    typedef boost::signals2::signal<void (void)> empty_signal;
    empty_signal on_queue_empty;
};
