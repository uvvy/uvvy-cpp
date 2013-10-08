//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/positional_options.hpp>
#include <queue>
#include <mutex>
#include "host.h"
#include "stream.h"
#include "server.h"
#include "logging.h"
#include "opus.h"
#include "RtAudio.h"
#include "settings_provider.h"

using namespace std;
using namespace ssu;

// Receive packets from remote
// Opus-decode and playback
class audio_receiver
{
    OpusDecoder *decstate{0};
    size_t framesize{0};
    int rate{0};
    std::mutex queue_mutex;
    std::queue<byte_array> packet_queue;
    shared_ptr<stream> stream_;

public:
    static constexpr int nChannels = 1;

    audio_receiver(shared_ptr<stream> stream)
        : stream_(stream)
    {
        int error = 0;
        decstate = opus_decoder_create(48000, nChannels, &error);
        assert(decstate);
        assert(!error);

        opus_decoder_ctl(decstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms

        stream_->on_ready_read_datagram.connect(
            boost::bind(&audio_receiver::on_packet_received, this));
    }

    ~audio_receiver()
    {
        opus_decoder_destroy(decstate); decstate = 0;
    }

    /**
     * Decode a packet into the provided buffer; decode a missing frame if queue is empty.
     * @param[out] decoded_packet Buffer for decoded packet.
     * @param[in] max_frames Maximum number of floating point frames in provided buffer.
     */
    void get_packet(float* decoded_packet, size_t max_frames)
    {
        logger::debug() << "get_packet";
        std::unique_lock<std::mutex> lock(queue_mutex);
        assert(max_frames == framesize);

        if (packet_queue.size() > 0)
        {
            byte_array pkt = packet_queue.front();
            packet_queue.pop();
            lock.unlock();
            int len = opus_decode_float(decstate, (unsigned char*)pkt.data()+4, pkt.size()-4, decoded_packet, framesize, /*decodeFEC:*/0);
            assert(len > 0);
            assert(len == int(framesize));
            logger::debug() << "get_packet decoded frame of size " << pkt.size() << " into " << len << " frames";
        } else {
            lock.unlock();
            // "decode" a missing frame
            int len = opus_decode_float(decstate, NULL, 0, decoded_packet, framesize, /*decodeFEC:*/0);
            assert(len > 0);
            logger::debug() << "get_packet decoded missing frame of size " << len;
            // assert(len == framesize);
        }
    }

protected:
    /* Put received packet into receive queue */
    void on_packet_received()
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        // extract payload
        byte_array msg = stream_->read_datagram();
        logger::debug() << "received packet of size " << msg.size();
        packet_queue.push(msg);
    }
};

// Capture and opus-encode audio
// send it to remote endpoint
class audio_sender
{
    OpusEncoder *encstate{0};
    int framesize{0}, rate{0};
    shared_ptr<stream> stream_;

public:
    static constexpr int nChannels = 1;

    audio_sender(shared_ptr<stream> stream)
        : stream_(stream)
    {
        int error = 0;
        encstate = opus_encoder_create(48000, nChannels, OPUS_APPLICATION_VOIP, &error);
        assert(encstate);
        assert(!error);

        opus_encoder_ctl(encstate, OPUS_GET_SAMPLE_RATE(&rate));
        framesize = rate / 100; // 10ms

        opus_encoder_ctl(encstate, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encstate, OPUS_SET_VBR(1));
        opus_encoder_ctl(encstate, OPUS_SET_DTX(1));
    }

    ~audio_sender()
    {
        stream_->shutdown(stream::shutdown_mode::write);
        opus_encoder_destroy(encstate); encstate = 0;
    }

    // Called by rtaudio callback to encode and send packet.
    void send_packet(float* buffer, size_t nFrames)
    {
        logger::debug() << "send_packet framesize " << framesize << ", got nFrames " << nFrames;
        assert((int)nFrames == framesize);
        byte_array samplebuf(nFrames*sizeof(float)); // @todo Add channel and stream header space
        opus_int32 nbytes = opus_encode_float(encstate, buffer, nFrames, (unsigned char*)samplebuf.data()+4, nFrames*sizeof(float)-4);
        assert(nbytes > 0);
        stream_->write_datagram(samplebuf, stream::datagram_type::non_reliable);
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

        logger::debug() << "rtcallback["<<instance<<"] outputBuffer " << outputBuffer << ", inputBuffer " << inputBuffer << ", nframes " << nFrames;

        // An RtAudio "frame" is one sample per channel,
        // whereas our "frame" is one buffer worth of data (as in Speex).

#if 0
        if (inputBuffer && outputBuffer) {
            std::copy_n((char*)inputBuffer, nFrames*sizeof(float), (char*)outputBuffer);
        }
#else
        if (inputBuffer) {
            instance->sender_->send_packet((float*)inputBuffer, nFrames);
        }
        if (outputBuffer) {
            instance->receiver_->get_packet((float*)outputBuffer, nFrames);
        }
#endif
        return 0;
    }
};

namespace po = boost::program_options;

/**
 * Get the address to talk to over IPv6 from the command line.
 * Parses [ipv6::]:port or [ipv6] with default port 9660.
 */
int main(int argc, char* argv[])
{
    bool connect_out{false};
    std::string peer;
    int port = 9660;

    po::options_description desc("Program arguments");
    desc.add_options()
        ("peer,a", po::value<std::string>(), "Peer IPv6 address, can be ipv6, [ipv6] or [ipv6]:port")
        ("port,p", po::value<int>(&port)->default_value(9660), "Run service on this port, connect peer on this port")
        ("help", "Print this help message");
    po::positional_options_description p;
    p.add("peer", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("port"))
    {
        port = vm["port"].as<int>();
    }

    if (vm.count("peer"))
    {
        peer = vm["peer"].as<std::string>();
        if (peer.find("]:") != peer.npos)
        {
            // split port off
            port = boost::lexical_cast<int>(peer.substr(peer.find("]:")+2));//@todo ipv6 only
            peer = peer.substr(0, peer.find("]:")+1);
        }
        if (peer.find("[") == 0)
        {
            peer = peer.substr(1, peer.find("]")-1);
        }
        connect_out = true;
    }

    // try {
        peer_id eid; // dummy peer id for now
        auto settings = settings_provider::instance();
        shared_ptr<host> host(host::create(settings.get(), port));
        shared_ptr<stream> stream;
        shared_ptr<server> server;

        if (connect_out)
        {
            endpoint remote_ep(boost::asio::ip::address_v4::from_string(peer), port);
            logger::debug() << "Connecting to " << remote_ep;

            stream = make_shared<ssu::stream>(host);
            stream->connect_to(eid, "streaming", "opus", remote_ep);
        }
        else
        {
            logger::debug() << "Listening on port " << port;

            server = make_shared<ssu::server>(host);
            // server->on_new_connection.connect();
            bool listening = server->listen("streaming", "Streaming services", "opus", "OPUS Audio protocol");
            assert(listening);
        }
        // audio_receiver receiver(host);
        // host.bind_receiver(opus_magic, &receiver);

        // audio_sender sender(l, remote_ep);

        // audio_hardware hw(&sender, &receiver); // open streams and start io

        host->run_io_service();
    // }
    // catch(std::exception& e)
    // {
    //     std::cout << "EXCEPTION " << e.what() << std::endl;
    //     return -1;
    // }
}
