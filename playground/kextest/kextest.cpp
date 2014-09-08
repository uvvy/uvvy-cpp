#include <sodiumpp/sodiumpp.h>
#include <string>
#include <set>
#include <valarray>
#include <iostream>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/overwrite.hpp>
#include "arsenal/proquint.h"
#include "arsenal/base32x.h"
#include "arsenal/base64.h"
#include "arsenal/hexdump.h"

using namespace sodiumpp;
using namespace boost;
using namespace std;

typedef nonce<crypto_box_NONCEBYTES-8, 8> nonce64;
typedef nonce<crypto_box_NONCEBYTES-16, 16> nonce128;

const string helloPacketMagic    = "hellopkt";
const string cookiePacketMagic   = "cookipkt";
const string initiatePacketMagic = "init-pkt";

template <typename T>
iterator_range<typename T::iterator> subrange(T base, int start_offset, int end_offset)
{
    return iterator_range<typename T::iterator>(base.begin() + start_offset, base.begin() + end_offset);
}

// Initiator sends Hello and subsequently Initiate
class kex_initiator
{
    secret_key long_term_key;
    secret_key short_term_key;
    nonce64 hello_nonce;

public:
    kex_initiator()
    {}

    // Create and return a hello packet from the initiator
    string send_hello()
    {
        string packet(192, '\0');
        boost::overwrite(helloPacketMagic, subrange(packet, 0, 8));
        boost::overwrite(short_term_key.pk.get(), subrange(packet, 8, 32));
        boost::overwrite(hello_nonce.sequential(), subrange(packet, 104, 8));
        string boxed(64, '\0');
        boost::overwrite(long_term_key.pk.get(), subrange(boxed, 0, 32));
        string box = "";//box(boxed, short_term_key, server.long_term_key, hello_nonce);
        boost::overwrite(box, subrange(packet, 112, 80));
        return packet;
    }

    string got_cookie(string pkt) { return ""s; }
    string send_initiate() { return ""s; } // must get payload from client
    string send_message() { return ""s; } // must be in client
};

// Responder responds with Cookie and subsequently creates far-end session state.
class kex_responder
{
    secret_key long_term_key;
    secret_key short_term_key;
    secret_key minute_key;
    set<string> cookie_cache;

public:
    kex_responder()
    {}

    string got_hello(string pkt) { return ""s; }
    string send_cookie() { return ""s; }
    void got_initiate(string pkt) {} // end of negotiation
    string send_message(string pkt) { return ""s; }
};

int main(int argc, const char ** argv)
{
    kex_initiator client;
    kex_responder server;
    string msg;

    msg = client.send_hello();
    hexdump(msg);
    msg = server.got_hello(msg);
    msg = client.got_cookie(msg);
    server.got_initiate(msg);
    msg = client.send_message();
    msg = server.send_message(msg);
}
