//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <queue>
#include <mutex>
#include "link.h"
#include "protocol.h"
#include "logging.h"
#include "opus.h"
#include "RtAudio.h"

// Receive packets from remote
// Opus-decode and playback
class audio_receiver : public ssu::link_receiver
{
    OpusDecoder *decstate{0};
    int framesize{0}, rate{0};
    std::mutex queue_mutex;
    std::queue<byte_array> packet_queue;

public:
    static constexpr int nChannels = 1;

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
    byte_array get_packet()
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        byte_array decoded_packet(framesize*sizeof(float));

        if (packet_queue.size() > 0)
        {
            byte_array pkt = packet_queue.front();
            packet_queue.pop();
            lock.unlock();
            int len = opus_decode_float(decstate, (unsigned char*)pkt.data()+4, pkt.size()-4, (float*)decoded_packet.data(), framesize, /*decodeFEC:*/0);
            assert(len > 0);
            assert(len == framesize);
        } else {
            lock.unlock();
            // "decode" a missing frame
            int len = opus_decode_float(decstate, NULL, 0, (float*)decoded_packet.data(), framesize, /*decodeFEC:*/0);
            assert(len > 0);
            assert(len == framesize);
        }
        return decoded_packet;
    }

protected:
    /* Put received packet into receive queue */
    virtual void receive(const byte_array& msg, const ssu::link_endpoint& src) override
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        // extract payload
        packet_queue.push(msg);
    }
};

// Capture and opus-encode audio
// send it to remote endpoint
class audio_sender
{
    OpusEncoder *encstate{0};
    int framesize{0}, rate{0};
    ssu::link& link_;
    ssu::endpoint& ep_;

public:
    static constexpr int nChannels = 1;

    audio_sender(ssu::link& l, ssu::endpoint& e)
        : link_(l)
        , ep_(e)
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
        byte_array samplebuf(nFrames*sizeof(float));
        opus_int32 nbytes = opus_encode_float(encstate, buffer, framesize, (unsigned char*)samplebuf.data()+4, nFrames*sizeof(float)-4);
        assert(nbytes > 0);
        samplebuf.resize(nbytes+4);
        *reinterpret_cast<ssu::magic_t*>(samplebuf.data()) = ssu::stream_protocol::magic;
        link_.send(ep_, samplebuf);
    }
};

class audio_hardware
{
    RtAudio* audio_inst{0};
    audio_sender* sender_{0};
    audio_receiver* receiver_{0};

public:
    audio_hardware(audio_sender* sender, audio_receiver* receiver)
        : sender_(sender)
        , receiver_(receiver)
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
        inparam.deviceId = audio_inst->getDefaultInputDevice();
        inparam.nChannels = audio_sender::nChannels;
        outparam.deviceId = audio_inst->getDefaultOutputDevice();
        outparam.nChannels = audio_receiver::nChannels;
        unsigned int bufferFrames = 480;

        try {
            audio_inst->openStream(&outparam, &inparam, RTAUDIO_FLOAT32, 48000, &bufferFrames, rtcallback, this);
        }
        catch (RtError &error) {
            logger::warning() << "Couldn't open stream, " << error.what();
            return;
        }

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

private:
    static int rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double, RtAudioStreamStatus, void *userdata)
    {
        audio_hardware* instance = reinterpret_cast<audio_hardware*>(userdata);

        // A PortAudio "frame" is one sample per channel,
        // whereas our "frame" is one buffer worth of data (as in Speex).

        if (inputBuffer) {
            instance->sender_->send_packet((float*)inputBuffer, nFrames);
        }
        if (outputBuffer) {
            byte_array pkt = instance->receiver_->get_packet();
            // inefficient: should be writing directly into outputBuffer
            std::copy(pkt.data(), pkt.data()+pkt.size(), (float*)outputBuffer);
        }

        return 0;
    }
};

constexpr ssu::magic_t opus_magic = 0x4f505553;

int main()
{
    ssu::link_host_state host;
    ssu::endpoint local_ep(boost::asio::ip::udp::v4(), 9660);
    boost::asio::io_service io_service;
    ssu::udp_link l(io_service, local_ep, host);

    audio_receiver receiver;
    host.bind_receiver(opus_magic, &receiver);

    audio_sender sender(l, local_ep);

    audio_hardware hw(&sender, &receiver); // open streams and start io

    io_service.run();
}
