#pragma once

namespace ssu {
namespace negotiation {

/**
 * This abstract base class manages the responder side of the key exchange.
 * It uses link_receiver interface as base to receive negotiation protocol control messages
 * and respond to incoming key exchange requests.
 */
class key_responder : public link_receiver
{
public:
    virtual void receive(const byte_array& msg, const link_endpoint& src);
};

} // namespace negotiation
} // namespace ssu
