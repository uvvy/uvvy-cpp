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
#include "audio_service.h"
#include "opus.h"
#include "RtAudio.h"
#include "stream.h"
#include "server.h"
#include "logging.h"
#include "algorithm.h"

// Set to 1 if you want to console-log in realtime thread.
#define REALTIME_CRIME 0

// Set to 1 if you want to generate gnuplot file of delays.
#define DELAY_PLOT 0

namespace pt = boost::posix_time;
using namespace std;
using namespace ssu;

static const std::string service_name{"streaming"};
static const std::string protocol_name{"opus"};

static const pt::ptime epoch{boost::gregorian::date(1970, boost::gregorian::Jan, 1)};

//=================================================================================================
// plotfile
//=================================================================================================

/**
 * Record timing information into a gnuplot-compatible file.
 */
class plotfile
{
#if DELAY_PLOT
    mutex m;
    ofstream out_;
public:
    plotfile()
        : out_("timeplot.dat", ios::out|ios::trunc|ios::binary)
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
        unique_lock<mutex> lock(m);
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
    mutex queue_mutex_;
    deque<byte_array> packet_queue_;
    shared_ptr<stream> stream_;
    plotfile plot_;
    int64_t time_difference_{0};

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
        unique_lock<mutex> lock(queue_mutex_);
        assert(max_frames == frame_size_);

        if (packet_queue_.size() > 0)
        {
            byte_array pkt = packet_queue_.front();
            packet_queue_.pop_front();
            lock.unlock();

            // log_packet_delay(pkt);
            logger::file_dump dump(pkt, "opus packet before decode");
    
            int len = opus_decode_float(decode_state_, (unsigned char*)pkt.data()+8, pkt.size()-8,
                decoded_packet, frame_size_, /*decodeFEC:*/0);
            assert(len > 0);
            assert(len == int(frame_size_));
            if (len != int(frame_size_)) {
                logger::warning() << "Short decode, decoded " << len << " frames, required "
                    << frame_size_;
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
        lock_guard<mutex> lock(queue_mutex_);
        // extract payload
        byte_array msg = stream_->read_datagram();
#if REALTIME_CRIME
        logger::debug() << "received packet of size " << msg.size();
#endif

        if (!time_difference_) {
            time_difference_ = (pt::microsec_clock::universal_time() - epoch).total_milliseconds()
                - msg.as<big_int64_t>()[0];
        }

        log_packet_delay(msg);

        packet_queue_.push_back(msg);
        sort(packet_queue_.begin(), packet_queue_.end(),
        [](byte_array const& a, byte_array const& b)
        {
            int64_t tsa = a.as<big_int64_t>()[0];
            int64_t tsb = b.as<big_int64_t>()[0];
            return tsa < tsb;
        });

        // Packets should be in strictly sequential order
        for (size_t i = 1; i < packet_queue_.size(); ++i)
        {
            int64_t tsa = packet_queue_[i-1].as<big_int64_t>()[0];
            int64_t tsb = packet_queue_[i].as<big_int64_t>()[0];
            if (tsb - tsa > 10) {
                logger::warning() << "Discontinuity in audio stream: packets are "
                    << (tsb - tsa) << "ms apart.";
            }
        }
    }

    void log_packet_delay(byte_array const& pkt)
    {
#if DELAY_PLOT
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
        int64_t ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
        samplebuf.as<big_int64_t>()[0] = ts;
        // Ideally, an ack packet would contain ts info at the receiving side for this packet.

        // @todo Perform encode in a separate thread, not capture thread.
        //async{
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

        open_audio();
    }

    ~audio_hardware()
    {
        close_audio();
        delete audio_inst; audio_inst = 0;
    }

    void open_audio()
    {
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
            throw;
        }
    }

    void close_audio()
    {
        try {
            audio_inst->closeStream();
        }
        catch (RtError &error) {
            logger::warning() << "Couldn't close stream, " << error.what();
            throw;
        }
    }

    void start_audio()
    {
        audio_inst->startStream();
    }

    void stop_audio()
    {
        audio_inst->stopStream();
    }

    void new_connection(shared_ptr<server> server)
    {
        auto stream = server->accept();
        if (!stream)
            return;

        logger::info() << "New incoming connection from " << stream->remote_host_id();
        streaming(stream);

        stream->on_link_up.connect([this] {
            start_audio();
        });

        if (stream->is_link_up()) {
            logger::debug() << "Incoming stream is ready, start sending immediately.";
            audio_inst->startStream();
        }
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
            copy_n((char*)inputBuffer, nFrames*sizeof(float), (char*)outputBuffer);
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

//=================================================================================================
// audio_service
//=================================================================================================

class audio_service::private_impl
{
public:
    std::shared_ptr<ssu::host> host_;
    audio_receiver receiver;
    audio_sender sender;
    audio_hardware hw;
    shared_ptr<stream> stream;
    shared_ptr<server> server;

    private_impl(std::shared_ptr<ssu::host> host)
        : host_(host)
        , receiver()
        , sender(host)
        , hw(&sender, &receiver)
    {}
};

audio_service::audio_service(std::shared_ptr<ssu::host> host)
    : pimpl_(make_shared<private_impl>(host))
{}

audio_service::~audio_service()
{}

void audio_service::establish_outgoing_session(peer_id const& eid,
    std::vector<std::string> ep_hints)
{
    logger::info() << "Connecting to " << eid;

    pimpl_->stream = make_shared<ssu::stream>(pimpl_->host_);
    pimpl_->hw.streaming(pimpl_->stream);
    pimpl_->stream->on_link_up.connect([this] {
        on_session_started();
        pimpl_->hw.start_audio();
    });
    pimpl_->stream->connect_to(eid, service_name, protocol_name);

    if (!ep_hints.empty())
    {
        for (auto epstr : ep_hints)
        {
            // @todo Allow specifying a port too.
            // @todo Allow specifying a DNS name for endpoint.
            ssu::endpoint ep(boost::asio::ip::address::from_string(epstr),
                stream_protocol::default_port);
            logger::debug() << "Connecting at location hint " << ep;
            pimpl_->stream->connect_at(ep);
        }
    }
}

void audio_service::listen_incoming_session()
{
    pimpl_->server = make_shared<ssu::server>(pimpl_->host_);
    pimpl_->server->on_new_connection.connect([this]
    {
        on_session_started();
        pimpl_->hw.new_connection(pimpl_->server);
    });
    bool listening = pimpl_->server->listen(service_name, "Streaming services",
                                            protocol_name, "OPUS Audio protocol");
    assert(listening);
    if (!listening) {
        throw runtime_error("Couldn't set up server listening to streaming:opus");
    }
}
