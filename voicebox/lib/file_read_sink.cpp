#include "logging.h"
#include "file_read_sink.h"

using namespace std;

namespace voicebox {

file_read_sink::file_read_sink(std::string const& filename)
    : filename_(filename)
{}

void file_read_sink::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << " " << enabling;
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
    }
}

void file_read_sink::produce_output(byte_array& buffer)
{
    if (!file_.is_open())
        return;

    short samples[frame_size() * num_channels()];
    off_t off = 0;
    size_t nbytesToRead = frame_size() * num_channels() * sizeof(short);

    file_.seekg(offset_);

    while ((nbytesToRead > 0) and file_)
    {
        logger::debug() << "Reading " << nbytesToRead << " bytes from file.";
        file_.read((char*)&samples[off], nbytesToRead);
        size_t nread = file_.gcount();
        logger::debug() << "Read    " << nbytesToRead << " bytes actually.";
        assert(!(nread % 2));
        offset_ += nread;
        off += nread/2;
        nbytesToRead -= nread;
        assert(nbytesToRead >= 0);

        if (nread <= nbytesToRead)
        {
            // Loop the file
            logger::debug() << "File ran out, restarting. Remaining to read " << nbytesToRead;
            offset_ = 0;
            file_.seekg(offset_);// clear eof() condition
        }
    }

    buffer.resize(frame_bytes());
    for (unsigned int i = 0; i < frame_size() * num_channels(); ++i) {
        buffer.as<float>()[i] = float(samples[i]) / 32767.0;
    }
}

} // voicebox namespace
