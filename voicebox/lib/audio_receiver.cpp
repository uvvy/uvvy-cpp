//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "logging.h"
#include "audio_receiver.h"
#include "audio_service.h"

namespace pt = boost::posix_time;
using namespace std;
using namespace ssu;

static const pt::ptime epoch{boost::gregorian::date(1970, boost::gregorian::Jan, 1)};

constexpr int audio_receiver::nChannels;

audio_receiver::audio_receiver()
{
    int error{0};
    decode_state_ = opus_decoder_create(48000, nChannels, &error);
    assert(decode_state_);
    assert(!error);

    opus_decoder_ctl(decode_state_, OPUS_GET_SAMPLE_RATE(&rate_));
    frame_size_ = rate_ / 100; // 10ms
}

audio_receiver::~audio_receiver()
{
    if (stream_) {
        stream_->shutdown(stream::shutdown_mode::read);
    }
    opus_decoder_destroy(decode_state_); decode_state_ = 0;
}

void audio_receiver::streaming(shared_ptr<stream> stream)
{
    stream_ = stream;
    stream_->on_ready_read_datagram.connect([this]{ on_packet_received(); });
}

/**
 * Decode a packet into the provided buffer; decode a missing frame if queue is empty.
 * @param[out] decoded_packet Buffer for decoded packet.
 * @param[in] max_frames Maximum number of floating point frames in provided buffer.
 */
void audio_receiver::get_packet(float* decoded_packet, size_t max_frames)
{
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "get_packet";
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
        logger::debug(TRACE_DETAIL) << "get_packet decoded frame of size " << pkt.size()
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
        logger::debug(TRACE_DETAIL) << "get_packet decoded missing frame of size " << len;
#endif
        // assert(len == frame_size_);
    }
}

/* Put received packet into receive queue */
void audio_receiver::on_packet_received()
{
    lock_guard<mutex> lock(queue_mutex_);
    // extract payload
    byte_array msg = stream_->read_datagram();
#if REALTIME_CRIME
    logger::debug(TRACE_ENTRY) << "received packet of size " << msg.size();
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
            logger::warning(TRACE_DETAIL) << "Discontinuity in audio stream: packets are "
                << (tsb - tsa) << "ms apart.";
        }
    }
}

void audio_receiver::log_packet_delay(byte_array const& pkt)
{
#if DELAY_PLOT
    int64_t ts = pkt.as<big_int64_t>()[0];
    int64_t local_ts = (pt::microsec_clock::universal_time() - epoch).total_milliseconds();
    // logger::info() << "Packet ts " << ts << ", local ts " << local_ts << ", play difference "
        // << fabs(local_ts - ts);

    plot_.dump(ts, local_ts);
#endif
}
