#include "negotiation/key_responder.h"
#include "negotiation/key_message.h"

namespace ssu {
namespace negotiation {

void key_responder::receive(byte_array& msg, boost::archive::binary_iarchive& ia, const link_endpoint& src)
{
	key_message msg;
	ia >> msg;
};

} // namespace negotiation
} // namespace ssu
