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
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <queue>
#include <mutex>
#include "host.h"
#include "stream.h"
#include "server.h"
#include "logging.h"
#include "opus.h"
#include "RtAudio.h"
#include "algorithm.h"
#include "settings_provider.h"
#include "upnpclient.h"

#include "private/regserver_client.h" // @fixme Testing only.
constexpr uint16_t regserver_port = uia::routing::internal::REGSERVER_DEFAULT_PORT;

// Set to 1 if you want to console-log in realtime thread.
#define REALTIME_CRIME 0

// Set to 1 if you want to generate gnuplot file of delays.
#define DELAY_PLOT 0

namespace pt = boost::posix_time;
namespace po = boost::program_options;
using namespace std;
using namespace ssu;

/**
 * Record timing information into a gnuplot-compatible file.
 */
class plotfile
{
#if DELAY_PLOT
    std::mutex m;
    std::ofstream out_;
public:
    plotfile()
        : out_("timeplot.dat", std::ios::out|std::ios::trunc|std::ios::binary)
    {
        out_ << "# gnuplot data for packet timing\r\n"
             << "# ts\tlocal_ts\tdifference\r\n";
    }

    ~plotfile()
    {
        out_ << "\r\n\r\n"; // gnuplot end marker
        out_.close();
    }

    void dump(int64_t ts, int64_t local_ts)
    {
        std::unique_lock<std::mutex> lock(m);
        out_ << ts << '\t' << local_ts << '\t' << fabs(local_ts - ts) << "\r\n";
    }
#endif
};

//=================================================================================================
// audio_receiver
//=================================================================================================

/**
 * Receive packets from remote, Opus-decode and playback.
 */
class audio_receiver
{
    OpusDecoder *decode_state_{0};
    size_t frame_size_{0};
    int rate_{0};
    std::mutex queue_mutex_;
    std::queue<byte_array> packet_queue_;
    shared_ptr<stream> stream_;
    plotfile plot_;

public:
    static constexpr int nChannels{1};

    audio_receiver()
    {
        int error{0};
        decode_state_ = opus_decoder_create(48000, nChannels, &error);
        assert(decode_state_);
        assert(!error);

        opus_decoder_ctl(decode_state_, OPUS_GET_SAMPLE_RATE(&rate_));
        frame_size_ = rate_ / 100; // 10ms
    }

    ~audio_receiver()
    {
        if (stream_) {
            stream_->shutdown(stream::shutdown_mode::read);
        }
        opus_decoder_destroy(decode_state_); decode_state_ = 0;
    }

    void streaming(shared_ptr<stream> stream)
    {
        stream_ = stream;
        stream_->on_ready_read_datagram.connect([this]{ on_packet_received(); });
    }

    /**
     * Decode a packet into the provided buffer; decode a missing frame if queue is empty.
     * @param[out] decoded_packet Buffer for decoded packet.
     * @param[in] max_frames Maximum number of floating point frames in provided buffer.
     */
    void get_packet(float* decoded_packet, size_t max_frames)
    {
#if REALTIME_CRIME
        logger::debug() << "get_packet";
#endif
        std::unique_lock<std::mutex> lock(queue_mutex_);
        assert(max_frames == frame_size_);

        if (packet_queue_.size() > 0)
        {
            byte_array pkt = packet_queue_.front();
            packet_queue_.pop();
            lock.unlock();

            // log_packet_delay(pkt);
            logger::file_dump dump(pkt, "opus packet before decode");
    
            int len = opus_decode_float(decode_state_, (unsigned char*)pkt.data()+8, pkt.size()-8,
                decoded_packet, frame_size_, /*decodeFEC:*/0);
            assert(len > 0);
            assert(len == int(frame_size_));
            if (len != int(frame_size_)) {
                logger::warning() << "Short decode, decoded " << len << " frames, required " << frame_size_;
            }
#if REALTIME_CRIME
            logger::debug() << "get_packet decoded frame of size " << pkt.size()
                << " into " << len << " frames";
#endif
        } else {
            lock.unlock();
            // "decode" a missing frame
            int len = opus_decode_float(decode_state_, NULL, 0, decoded_packet,
                frame_size_, /*decodeFEC:*/0);
            assert(len > 0);
            if (len <= 0) {
                logger::warning() << "Couldn't decode missing frame, returned " << len;
            }
#if REALTIME_CRIME
            logger::debug() << "get_packet decoded missing frame of size " << len;
#endif
            // assert(len == frame_size_);
        }
    }

protected:
    /* Put received packet into receive queue */
    void on_packet_received()
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        // extract payload
        byte_array msg = stream_->read_datagram();
#if REALTIME_CRIME
        logger::debug() << "received packet of size " << msg.size();
#endif
        
        log_packet_delay(msg);

        packet_queue_.push(msg);
    }

    void log_packet_delay(byte_array const& pkt)
    {
#if DELAY_PLOT
        pt::ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));
        int64_t ts = pkt.as<big_int64_t>()[0];
        int64_t local_ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
        // logger::info() << "Packet ts " << ts << ", local ts " << local_ts << ", play difference "
            // << fabs(local_ts - ts);

        plot_.dump(ts, local_ts);
#endif
    }
};

//=================================================================================================
// audio_sender
//=================================================================================================

/**
 * Capture and Opus-encode audio, send it to remote endpoint.
 */
class audio_sender
{
    OpusEncoder *encode_state_{0};
    int frame_size_{0}, rate_{0};
    shared_ptr<stream> stream_;
    boost::asio::strand strand_;

public:
    static constexpr int nChannels{1};

    audio_sender(shared_ptr<host> host)
        : strand_(host->get_io_service())
    {
        int error{0};
        encode_state_ = opus_encoder_create(48000, nChannels, OPUS_APPLICATION_VOIP, &error);
        assert(encode_state_);
        assert(!error);

        opus_encoder_ctl(encode_state_, OPUS_GET_SAMPLE_RATE(&rate_));
        frame_size_ = rate_ / 100; // 10ms

        opus_encoder_ctl(encode_state_, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(encode_state_, OPUS_SET_VBR(1));
        opus_encoder_ctl(encode_state_, OPUS_SET_DTX(1));
    }

    ~audio_sender()
    {
        if (stream_) {
            stream_->shutdown(stream::shutdown_mode::write);
        }
        opus_encoder_destroy(encode_state_); encode_state_ = 0;
    }

    /**
     * Use given stream for sending out audio packets.
     * @param stream Stream handed in from server->accept().
     */
    void streaming(shared_ptr<stream> stream)
    {
        stream_ = stream;
    }

    // Called by rtaudio callback to encode and send packet.
    void send_packet(float* buffer, size_t nFrames)
    {
#if REALTIME_CRIME
        logger::debug() << "send_packet frame size " << frame_size_
            << ", got nFrames " << nFrames;
#endif
        assert((int)nFrames == frame_size_);
        byte_array samplebuf(nFrames*sizeof(float)+8);

        // Timestamp the packet with our own clock reading.
        pt::ptime epoch(boost::gregorian::date(1970,boost::gregorian::Jan,1));
        int64_t ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
        samplebuf.as<big_int64_t>()[0] = ts;
        // Ideally, an ack packet would contain ts info at the receiving side for this packet.
        // @todo Implement the RTT calculation in ssu::stream!

        // @todo Perform encode in a separate thread, not capture thread.
        //std::async{
        opus_int32 nbytes = opus_encode_float(encode_state_, buffer, nFrames,
            (unsigned char*)samplebuf.data()+8, nFrames*sizeof(float));
        assert(nbytes > 0);
        samplebuf.resize(nbytes+8);
        logger::file_dump dump(samplebuf, "encoded opus packet");
        strand_.post([this, samplebuf]{
            stream_->write_datagram(samplebuf, stream::datagram_type::non_reliable);
        });
        // }
    }
};

//=================================================================================================
// audio_hardware
//=================================================================================================

/**
 * Controls the audio layer and starts playback and capture.
 */
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
        unsigned int bufferFrames = 480; // 10ms

        try {
            audio_inst->openStream(&outparam, &inparam, RTAUDIO_FLOAT32, 48000, &bufferFrames,
                rtcallback, this);
        }
        catch (RtError &error) {
            logger::warning() << "Couldn't open stream, " << error.what();
            return;
        }
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

    void new_connection(shared_ptr<server> server)
    {
        auto stream = server->accept();
        if (!stream)
            return;

        logger::info() << "New incoming connection from " << stream->remote_host_id();
        streaming(stream);

        audio_inst->startStream();
    }

    void out_stream_ready()
    {
        audio_inst->startStream();
    }

    void streaming(shared_ptr<stream> stream)
    {
        sender_->streaming(stream);
        receiver_->streaming(stream);
    }

private:
    static int rtcallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double,
        RtAudioStreamStatus, void *userdata)
    {
        audio_hardware* instance = reinterpret_cast<audio_hardware*>(userdata);

#if REALTIME_CRIME
        logger::debug() << "rtcallback["<<instance<<"] outputBuffer " << outputBuffer
            << ", inputBuffer " << inputBuffer << ", nframes " << nFrames;
#endif

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

shared_ptr<upnp::UpnpIgdClient> traverse_nat(int main_port)
{
    shared_ptr<upnp::UpnpIgdClient> upnp = make_shared<upnp::UpnpIgdClient>();

    logger::info() << "Initialising UPnP";

    upnp->InitControlPoint();

    bool main_port_mapped{false}, regserver_port_mapped{false};

    if (upnp->IsAsync()) {
        upnp->SetNewMappingCallback([&](const int &port, const upnp::ProtocolType &protocol) {
            if (port == main_port) {
                main_port_mapped = true;
            } else if (port == regserver_port) {
                regserver_port_mapped = true;
            }
        });
    }

    bool all_added = true;
    all_added &= upnp->AddPortMapping(main_port, upnp::kTcp);
    all_added &= upnp->AddPortMapping(main_port, upnp::kUdp);
    all_added &= upnp->AddPortMapping(regserver_port, upnp::kTcp);
    all_added &= upnp->AddPortMapping(regserver_port, upnp::kUdp);

    if (upnp->IsAsync()) {
        logger::debug() << "Waiting...";
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }

    if (upnp->HasServices()) {
      logger::debug() << "External IP: " << upnp->GetExternalIpAddress();
      assert(all_added);
      if (upnp->IsAsync()) {
        assert(main_port_mapped and regserver_port_mapped);
      }
      logger::info() << "All UPnP mappings successful";
    } else {
      logger::warning() << "Sorry, no port mappings via UPnP possible";
    }

    return upnp;
}

/**
 * Get the address to talk to over IPv6 from the command line.
 * Parses [ipv6::]:port or [ipv6] with default port 9660.
 */
int main(int argc, char* argv[])
{
    bool connect_out{false};
    std::string peer;
    int port = stream_protocol::default_port;
    std::vector<std::string> location_hints;
    bool verbose_debug{false};

    po::options_description desc("Program arguments");
    desc.add_options()
        ("remote,r", po::value<std::string>(),
            "Peer EID as base32 identifier")
        ("endpoint,e", po::value<std::vector<std::string>>(&location_hints),
            "Endpoint location hint (ipv4 or ipv6 address), can be specified multiple times")
        ("port,p", po::value<int>(&port),
            "Run service on this port, connect peer on this port")
        ("verbose,v", po::bool_switch(&verbose_debug),
            "Print verbose output for debug")
        ("help",
            "Print this help message");
    po::positional_options_description p;
    p.add("remote", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (!verbose_debug) {
        logger::set_verbosity(logger::verbosity::info);
    }

    auto settings = settings_provider::instance();

    auto s_port = settings->get("port");
    if (!s_port.empty()) {
        port = boost::any_cast<long long>(s_port);
    }

    if (vm.count("port"))
    {
        port = vm["port"].as<int>();
    }

    if (vm.count("remote"))
    {
        peer = vm["remote"].as<std::string>();
        connect_out = true;
    }

    settings->set("port", (long long)port);
    settings->sync();

    // Shared ptr ensures nat is destroyed on exit...
    shared_ptr<upnp::UpnpIgdClient> nat(traverse_nat(port));

    shared_ptr<host> host(host::create(settings.get(), port));
    shared_ptr<stream> stream;
    shared_ptr<server> server;

    uia::routing::internal::regserver_client regclient(host.get());

    // Pull client profile from settings.
    auto s_client = settings->get("profile");
    uia::routing::client_profile client;
    if (!s_client.empty()) {
        uia::routing::client_profile client2(boost::any_cast<std::vector<char>>(s_client));
        client = client2;
    }
    client.set_endpoints(set_to_vector(host->active_local_endpoints()));
    for (auto kw : client.keywords()) {
        logger::debug() << "Keyword: " << kw;
    }
    regclient.set_profile(client);

    boost::any s_rs = settings->get("regservers");
    if (!s_rs.empty())
    {
        byte_array rs_ba(boost::any_cast<vector<char>>(s_rs));
        byte_array_iwrap<flurry::iarchive> read(rs_ba);
        vector<string> regservers;
        read.archive() >> regservers;
        for (auto server : regservers)
        {
            regclient.register_at(server);
        }
    }

    audio_receiver receiver;
    audio_sender sender(host);
    audio_hardware hw(&sender, &receiver);

    if (connect_out)
    {
        peer_id eid{peer};
        logger::info() << "Connecting to " << eid;

        stream = make_shared<ssu::stream>(host);
        hw.streaming(stream);
        stream->on_link_up.connect([&] { hw.out_stream_ready(); });
        stream->connect_to(eid, "streaming", "opus");

        if (!location_hints.empty())
        {
            for (auto epstr : location_hints)
            {
                // @todo Allow specifying a port too.
                // @todo Allow specifying a DNS name for endpoint.
                ssu::endpoint ep(boost::asio::ip::address::from_string(epstr), stream_protocol::default_port);
                logger::debug() << "Connecting at location hint " << ep;
                stream->connect_at(ep);
            }
        }
    }
    else
    {
        logger::info() << "Host " << host->host_identity().id() << " listening on port " << dec << port;

        server = make_shared<ssu::server>(host);
        server->on_new_connection.connect([&] { hw.new_connection(server); });
        bool listening = server->listen("streaming", "Streaming services",
                                        "opus", "OPUS Audio protocol");
        assert(listening);
        if (!listening) {
            throw std::runtime_error("Couldn't set up server listening to streaming:opus");
        }
    }

    host->run_io_service();
}
