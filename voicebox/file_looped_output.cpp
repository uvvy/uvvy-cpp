#include "logging.h"
#include "file_looped_output.h"

using namespace std;

file_looped_output::file_looped_output(string const& filename)
    : filename_(filename)
{}

void file_looped_output::set_enabled(bool enabling)
{
    logger::debug() << __PRETTY_FUNCTION__ << enabling << is_enabled();
    if (enabling and !is_enabled())
    {
        assert(!file_.is_open());
        file_.open(filename_, ios::in|ios::binary);
        // @todo Figure this info from the file header.
        set_frame_size(480);
        set_sample_rate(48000);

        offset_ = 0;

        super::set_enabled(true);
    }
    else if (!enabling and is_enabled())
    {
        super::set_enabled(false);

        file_.close();
    }
}

void file_looped_output::produce_output(float *samplebuf)
{
    if (!file_.is_open())
        return;
    logger::debug() << __PRETTY_FUNCTION__;

    short samples[frame_size()];
    off_t off = 0;
    size_t nbytesToRead = frame_size() * sizeof(short);

    file_.seekg(offset_);

    while (nbytesToRead > 0 and !file_.eof())
    {
        file_.read((char*)&samples[off], nbytesToRead);
        size_t nread = file_.gcount();
        assert(!(nread % 2));
        offset_ += nread;
        off += nread/2;
        nbytesToRead -= nread;
        assert(nbytesToRead >= 0);

        if (nread < nbytesToRead)
        {
            // Loop the file
            offset_ = 0;
            file_.seekg(offset_);// clear eof() condition
        }
    }
    for (int i = 0; i < frame_size(); ++i) {
        samplebuf[i] = (float)samples[i];
    }
}

