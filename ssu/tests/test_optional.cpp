#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/optional/optional.hpp>
#include "custom_optional.h"

using namespace std;

// @todo: add variant<> serialization
// @todo: add ?types serialization

//uint32_t chunk_size;
//uint32_t discriminator;
//opaque[chunk_size-4] struct type; - based on discriminator
// template <typename T>
// class sized_optional : public optional<T>
// {};

// ixstream& operator >> (ixstream& ixs, sized_optional<T>& so)
// {
//     byte_array ba;
//     ixs >> ba;
//     if (ba.is_empty())
//     {
//         return false;
//     }
//     binary_iarchive ia2(ba);
//     ia2 >> so; // discriminated enum
//     return ixs;
// }

// operator << (sized_optional<T>& so)
// {
//     if (!so.is_initialized())
//     {
//         uint32_t size = 0;
//         os << size;
//         return os;
//     }
//     omemorystream om;
//     om << *so;
//     uint32_t size = om.size();
//     os << size << om;
// }

//struct opaque_vararr {
//    std::vector<char> data;
//};

int main()
{
    uint32_t i = 0xabbadead;
    boost::optional<uint32_t> maybe_value;

    {
        std::ofstream os("testarchive.bin", std::ios_base::binary|std::ios_base::out|std::ios::trunc);
        boost::archive::binary_oarchive oa(os, boost::archive::no_header);
        assert(maybe_value.is_initialized() == false);
        oa & maybe_value;
        maybe_value = i;
        assert(maybe_value.is_initialized() == true);
        assert(*maybe_value == 0xabbadead);
        oa & maybe_value;
    }
    {
        std::ifstream is("testarchive.bin", std::ios_base::binary|std::ios_base::in);
        boost::archive::binary_iarchive ia(is, boost::archive::no_header);
        ia & maybe_value;
        assert(maybe_value.is_initialized() == false);
        ia & maybe_value;
        assert(maybe_value.is_initialized() == true);
        assert(*maybe_value == 0xabbadead);
    }
}
