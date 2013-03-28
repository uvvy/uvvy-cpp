#include "negotiation/key_responder.h"
#include "negotiation/key_message.h"

namespace ssu {
namespace negotiation {

void key_responder::receive(const byte_array& msg, const link_endpoint& src)
{
    boost::iostreams::filtering_istream in(boost::make_iterator_range(msg.as_vector()));
    boost::archive::binary_iarchive ia(in, boost::archive::no_header);
	key_message m;
	ia >> m;
};

} // namespace negotiation
} // namespace ssu
