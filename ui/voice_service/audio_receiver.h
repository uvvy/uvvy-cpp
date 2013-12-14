#pragma once

#include <memory>
#include <mutex>
#include <deque>
#include "opus.h"
#include "plotfile.h"
#include "byte_array.h"
#include "stream.h"

/**
 * Receive packets from remote, Opus-decode and playback.
 */
class audio_receiver
{
    OpusDecoder *decode_state_{0};
    size_t frame_size_{0};
    int rate_{0};
    std::mutex queue_mutex_;
    std::deque<byte_array> packet_queue_;
    std::shared_ptr<ssu::stream> stream_;
    plotfile plot_;
    int64_t time_difference_{0};

public:
    static constexpr int nChannels{1};

    audio_receiver();
    ~audio_receiver();

    void streaming(std::shared_ptr<ssu::stream> stream);

    /**
     * Decode a packet into the provided buffer; decode a missing frame if queue is empty.
     * @param[out] decoded_packet Buffer for decoded packet.
     * @param[in] max_frames Maximum number of floating point frames in provided buffer.
     */
    void get_packet(float* decoded_packet, size_t max_frames);

protected:
    /* Put received packet into receive queue */
    void on_packet_received();

    void log_packet_delay(byte_array const& pkt);
};
