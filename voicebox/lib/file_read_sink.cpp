#include "logging.h"
#include "voicebox/file_read_sink.h"

using namespace std;

namespace voicebox {

file_read_sink::file_read_sink(std::string const& filename)
    : filename_(filename)
{}

void file_read_sink::set_enabled(bool enabling)
{
    if (enabling and !is_enabled())
    {
        assert(!file_.is_open());
        file_.open(filename_, ios::in|ios::binary);
        offset_ = 0;

        logger::debug() << "File read sink enabled: rate " << sample_rate()
            << ", channels " << num_channels() << ", frame size " << frame_size();

        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);
        file_.close();
        logger::debug() << "File read sink disabled";
    }
}

void file_read_sink::produce_output(byte_array& buffer)
{
    if (!file_.is_open())
        return;

    size_t n_frames = frame_size() * num_channels();

    short samples[n_frames];
    off_t off = 0;
    size_t nbytesToRead = n_frames * sizeof(short);

    fill_n(samples, n_frames, 0);

    file_.seekg(offset_);

    while ((nbytesToRead > 0) and file_)
    {
        file_.read((char*)&samples[off], nbytesToRead);
        size_t nread = file_.gcount();
        assert(!(nread % 2));
        offset_ += nread;
        off += nread/2;
        nbytesToRead -= nread;
        assert(nbytesToRead >= 0);

        if (nread <= nbytesToRead or file_.eof())
        {
            // Loop the file
            offset_ = 0;
            file_.clear();
            file_.seekg(offset_);
        }
    }

    buffer.resize(frame_bytes());
    for (unsigned int i = 0; i < n_frames; ++i) {
        buffer.as<float>()[i] = float(samples[i]) / 32768.0;
    }
}

} // voicebox namespace
