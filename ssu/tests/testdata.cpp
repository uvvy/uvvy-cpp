#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/optional/optional.hpp>
#include "protocol.h"
#include "byte_array.h"
#include "custom_optional.h"
#include "xdr.h"

using namespace std;

/**
 * Generate a binary blob for testing key_message.h i/o functions.
 * Create a key message with dh_init1 and checksum_init chunks, add packet chunk of data afterwards.
 *
 * +-------+--------+-----------------------------------------------------------------+
 * | magic | count  | opaque                                                          |
 * +-------+--------+--------+-------+------------------------------------------------+
 *                  | length | discr |                                                |
 *                  +--------+-------+------------------------------------------------+
 *                     0xb4    0x21  | DHgroup | keymin | nhi | length | dhi | length |
 *                                   +---------+--------+-----+--------+-----+--------+
 *                                       0x1      0x10    32b
 */
int main()
{
    uint32_t magic = boost::endian2::big(stream_protocol::magic);
    uint32_t count = boost::endian2::big(1);
    uint32_t opaque_size = boost::endian2::big(0xb4);
    uint32_t disciminant = boost::endian2::big(0x21); // dh_init1
    uint32_t dh_group = boost::endian2::big(0x1);
    uint32_t keymin = boost::endian2::big(0x10);
    byte_array nhi;
    byte_array dhi;
    byte_array eidi;

    nhi.resize(32);
    for (int i = 0; i < 32; ++i)
        nhi[i] = rand();
    dhi.resize(128);
    for (int i = 0; i < 128; ++i)
        dhi[i] = rand();

    {
        std::ofstream os("testdata.bin", std::ios_base::binary|std::ios_base::out|std::ios::trunc);
        boost::archive::binary_oarchive oa(os, boost::archive::no_header);

        oa << magic;
        oa << count;
        oa << opaque_size;
        oa << disciminant;
        oa << dh_group;
        oa << keymin;
        xdr::encode_vector(oa, nhi, 32);
        xdr::encode_array(oa, dhi, 384);
        xdr::encode_array(oa, eidi, 256);
    }
}
