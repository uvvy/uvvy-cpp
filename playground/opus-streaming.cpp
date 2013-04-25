//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "link.h"
#include <opus.h>

class audio_sender;

class audio_hardware
{
    RtAudio* audio_inst{0};
    audio_sender* sender_{0};

    audio_hardware(audio_sender* sender)
        : sender_(sender)
    {
        try {
            audio_inst  = new RtAudio();
        }
        catch (RtError &error) {
            logger::warning() << "Can't initialize RtAudio library, " << error.what();
            return;
        }

        // Open the audio device
        RtAudio::StreamParameters inparam, outparam;
        inparam.deviceId = 0;
        inparam.nChannels = audio_sender::nChannels;
        outparam.deviceId = 0;
        outparam.nChannels = audio_receiver::nChannels;
        unsigned int bufferFrames = minframesize;

        try {
            audio_inst->openStream(&outparam, &inparam, RTAUDIO_FLOAT32, maxrate, &bufferFrames, rtcallback);
        }
        catch (RtError &error) {
            logger::warning() << "Couldn't open stream, " << error.what();
            return;
        }

        hwrate = maxrate;
        hwframesize = bufferFrames;

        logger::debug() << "Open resulting hwrate " << hwrate << ", framesize " << hwframesize;

        audio_inst->startStream();
    }

    ~audio_hardware()
    {
        try {
            audio_inst->closeStream();
        }
        catch (RtError &error) {
            logger::warning() << "Couldn't close stream, " << error.what();
        }
        delete audio_inst; audio_inst = 0;
    }

    int rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double, RtAudioStreamStatus, void *)
    {
        // A PortAudio "frame" is one sample per channel,
        // whereas our "frame" is one buffer worth of data (as in Speex).
        assert(nFrames == hwframesize);

        if (inputBuffer) {
            sender_->send_packet((float*)inputBuffer, nFrames);
        }
        if (outputBuffer) {
            mixout((float*)outputBuffer);
        }

        return 0;
    }
};

// Receive packets from remote
// Opus-decode and playback
class audio_receiver : public link_receiver
{
    OpusDecoder *decstate{0};
    int framesize{0}, rate{0};
    constexpr int nChannels = 1;

    audio_receiver()
    {
        int error = 0;
        decstate = opus_decoder_create(48000, nChannels, &error);
        assert(decstate);
        assert(!error);

        opus_decoder_ctl(decstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 50; // 20ms
    }

    ~audio_receiver()
    {
        opus_decoder_destroy(decstate); decstate = 0;
    }

    /* Get and return a packet, decoding it; decode a missing frame if queue is empty */
    void get_packet()
    {
        unsigned int len = opus_decode_float(decstate, (unsigned char*)msg.data(), msg.size(), samplebuf, framesize, /*decodeFEC:*/1);
        // Q_ASSERT(len > 0);
        // Q_ASSERT(len == frameSize());

        // "decode" a missing frame
        // int len = opus_decode_float(decstate, NULL, 0, samplebuf, frameSize(), /*decodeFEC:*/1);
    }

protected:
    /* Put received packet into receive queue */
    void receive(const byte_array& msg, const link_endpoint& src) override
    {
    }
};

// Capture and opus-encode audio
// send it to remote endpoint
class audio_sender
{
    OpusEncoder *encstate{0};
    int framesize{0}, rate{0};
    constexpr int nChannels = 1;
    ssu::link& link_;

    audio_sender(ssu::link& l) : link_(l)
    {
        int error = 0;
        encstate = opus_encoder_create(48000, nChannels, OPUS_APPLICATION_VOIP, &error);
        assert(encstate);
        assert(!error);

        opus_encoder_ctl(encstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 50; // 20ms

        opus_encoder_ctl(encstate, OPUS_SET_VBR(1));
        opus_encoder_ctl(encstate, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encstate, OPUS_SET_DTX(1));
    }

    ~audio_sender()
    {
        opus_encoder_destroy(encstate); encstate = 0;
    }

    // Called by rtaudio callback to encode and send packet.
    void send_packet(float* buffer, size_t nFrames)
    {
        int nbytes = opus_encode_float(encstate, samplebuf, framesize, (unsigned char*)buffer, nFrames*sizeof(float));
        link_.send(local_ep, msg);
    }
};

void main()
{
    ssu::link_host_state host;
    ssu::endpoint local_ep(boost::asio::ip::udp::v4(), 9660);
    boost::asio::io_service io_service;
    ssu::udp_link l(io_service, local_ep, host);

    audio_receiver receiver;
    host.bind_receiver(stream_protocol::magic, &receiver);

    audio_sender sender(l);

    audio_hardware hw(&sender); // open streams and start io

    io_service.run();
}
