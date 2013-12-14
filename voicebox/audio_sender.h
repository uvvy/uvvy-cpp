#pragma once

#include "opus.h"
#include "host.h"
#include "stream.h"

/**
 * Capture and Opus-encode audio, send it to remote endpoint.
 */
class audio_sender
{
    OpusEncoder *encode_state_{0};
    int frame_size_{0}, rate_{0};
    std::shared_ptr<ssu::stream> stream_;
    boost::asio::strand strand_;

public:
    static constexpr int nChannels{1};

    audio_sender(std::shared_ptr<ssu::host> host);
    ~audio_sender();

    /**
     * Use given stream for sending out audio packets.
     * @param stream Stream handed in from server->accept().
     */
    void streaming(std::shared_ptr<ssu::stream> stream);

    // Called by rtaudio callback to encode and send packet.
    void send_packet(float* buffer, size_t nFrames);
};
