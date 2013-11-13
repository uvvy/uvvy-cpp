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
#include <boost/format.hpp>
#include "flurry.h"
#include "logging.h"
#include "byte_array_wrap.h"
#include "opus.h"

using namespace std;
namespace po = boost::program_options;

/**
 * Record information into a gnuplot-compatible file.
 */
class plotfile
{
    std::mutex m;
    std::ofstream out_;
public:
    plotfile(std::string const& plotname)
        : out_(plotname, std::ios::out|std::ios::trunc|std::ios::binary)
    {
        out_ << "# gnuplot data for packet dump tracing\r\n"
             << "# seq\tpacket_size\tdecoded size\r\n";
    }

    ~plotfile()
    {
        out_ << "\r\n\r\n"; // gnuplot end marker
        out_.close();
    }

    void dump(int64_t seq, int64_t size, int64_t out_size)
    {
        std::unique_lock<std::mutex> lock(m);
        out_ << seq << '\t' << size << '\t' << out_size << "\r\n";
    }
};

struct Decoder
{
    OpusDecoder *decode_state_{0};
    size_t frame_size_{0};
    int rate_{0};

    Decoder()
    {
        int error{0};
        decode_state_ = opus_decoder_create(48000, /*nChannels:*/1, &error);
        assert(decode_state_);
        assert(!error);

        opus_decoder_ctl(decode_state_, OPUS_GET_SAMPLE_RATE(&rate_));
        frame_size_ = rate_ / 100; // 10ms
    }

    ~Decoder()
    {
        opus_decoder_destroy(decode_state_);
        decode_state_ = 0;
    }

    byte_array decode(byte_array const& pkt)
    {
        byte_array decoded_packet(frame_size_*sizeof(float));
        int len = opus_decode_float(decode_state_, (unsigned char*)pkt.data()+8, pkt.size()-8,
            (float*)decoded_packet.data(), frame_size_, /*decodeFEC:*/0);
        assert(len > 0);
        assert(len == int(frame_size_));
        if (len != int(frame_size_)) {
            logger::warning() << "Short decode, decoded " << len << " frames, required " << frame_size_;
        }
        return decoded_packet;
    }
};

int main(int argc, char** argv)
{
    std::string filename;

    po::options_description desc("Log file analyzer");
    desc.add_options()
        ("filename,f", po::value<std::string>(&filename)->default_value("dump.bin"),
            "Name of the log dump file")
        ("help,h",
            "Print this help message");
    po::positional_options_description p;
    p.add("filename", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    byte_array data;
    std::ifstream in(filename, std::ios::in|std::ios::binary);
    flurry::iarchive ia(in);

    plotfile plot("packetsizes.plot");
    uint32_t old_seq = 0;
    Decoder dec;
    std::ofstream os("decoded.mono.flt", std::ios::out|std::ios::trunc|std::ios::binary);
    while (ia >> data) {
        // std::string what, stamp;
        // byte_array blob;
        // byte_array_iwrap<flurry::iarchive> read(data);
        // read.archive() >> what >> stamp >> blob;
        // cout << "*** BLOB " << blob.size() << " bytes *** " << stamp << ": " << what << endl;
        // hexdump(blob);

        uint32_t magic = data.as<big_uint32_t>()[0];
        if ((magic & 0xff000000) == 0) {
            continue; // Ignore control packets
        }
        uint32_t seq = magic & 0xffffff;
        if (old_seq != seq) {
            if (seq - old_seq > 1) {
                logger::warning() << "Non-consecutive sequence numbers " << old_seq << "->" << seq;
            }
            old_seq = seq;
            continue;
        }
        byte_array out = dec.decode(data);
        plot.dump(seq, data.size(), out.size());
        os.write(out.data(), out.size());
    }
}
