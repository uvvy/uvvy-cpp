#include <sodiumpp/sodiumpp.h>
#include <string>
#include <set>
#include <valarray>
#include <iostream>
#include <boost/range/algorithm.hpp>
#include "arsenal/proquint.h"
#include "arsenal/base32x.h"
#include "arsenal/base64.h"
#include "arsenal/hexdump.h"
#include "arsenal/subrange.h"

using namespace sodiumpp;
using namespace boost;
using namespace std;

typedef nonce<crypto_box_NONCEBYTES-8, 8> nonce64;
typedef nonce<crypto_box_NONCEBYTES-16, 16> nonce128;
typedef source_nonce<crypto_box_NONCEBYTES> recv_nonce;

const string helloPacketMagic     = "hellopkt";
const string cookiePacketMagic    = "cookipkt";
const string initiatePacketMagic  = "init-pkt";
const string helloNoncePrefix     = "hello-nonce-----";
const string minuteKeyNoncePrefix = "minute-k";
const string cookieNoncePrefix    = "CurveCPK";

// Initiator sends Hello and subsequently Initiate
class kex_initiator
{
    secret_key long_term_key;
    secret_key short_term_key;
    struct server {
        string long_term_key; // == kex_responder.long_term_key.pk
    } server;

public:
    kex_initiator()
    {}

    void set_peer_pk(string pk) { server.long_term_key = pk; }

    // Create and return a hello packet from the initiator
    string send_hello()
    {
        boxer<nonce64> seal(server.long_term_key, short_term_key, helloNoncePrefix);

        string packet(192, '\0');
        subrange(packet, 0, 8) = helloPacketMagic;
        subrange(packet, 8, 32) = short_term_key.pk.get();
        string boxed(64, '\0');
        subrange(boxed, 0, 32) = long_term_key.pk.get();
        subrange(packet, 112, 80) = seal.box(boxed);
        subrange(packet, 104, 8) = seal.nonce_sequential();
        return packet;
    }

    string got_cookie(string pkt) { return send_initiate(); }
    string send_message() { return ""s; } // must be in client

private:
    string send_initiate() { return ""s; } // must get payload from client
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

    string long_term_pk() const { return long_term_key.pk.get(); }

    string got_hello(string pkt)
    {
        assert(pkt.size() == 192);
        assert(subrange(pkt, 0, 8) == helloPacketMagic);

        string clientKey = subrange(pkt, 8, 32);

        string nonce(24, '\0');
        subrange(nonce, 0, 16) = helloNoncePrefix;
        subrange(nonce, 16, 8) = subrange(pkt, 104, 8);

        string unbox(64, '\0');
        unboxer<recv_nonce> unseal(clientKey, long_term_key, nonce);

        string open = unseal.unbox(subrange(pkt, 112, 80));
        hexdump(open);

        // Open box contains client's long-term public key which we should check against:
        //  a) blacklist
        //  b) already initiated connection list

        // It could be beneficial to have asymmetric connection channels, it will ease
        // connection setup handling.

        return send_cookie(clientKey);
    }

    void got_initiate(string pkt) {} // end of negotiation
    string send_message(string pkt) { return ""s; }

private:
    string send_cookie(string clientKey)
    {
        string packet(8+16+144, '\0');
        string cookie(96, '\0');
        secret_key sessionKey; // Generate short-term server key

        // Client short-term public key
        subrange(cookie, 16, 32) = clientKey;
        // Server short-term secret key
        subrange(cookie, 48, 32) = sessionKey.get();

        // minute-key secretbox nonce
        random_nonce<8> minuteKeyNonce(minuteKeyNoncePrefix);
        subrange(cookie, 16, 80)
            = crypto_secretbox(subrange(cookie, 16, 64), minuteKeyNonce.get(), minute_key.get());

        // Compressed cookie nonce
        subrange(cookie, 0, 16) = minuteKeyNonce.sequential();

        boxer<random_nonce<8>> seal(clientKey, long_term_key, cookieNoncePrefix);

        // Server short-term public key + cookie
        // Box the cookies
        string box = seal.box(sessionKey.pk.get() + cookie);
        assert(box.size() == 96+32+16);

        subrange(packet, 0, 8) = cookiePacketMagic;
        subrange(packet, 8, 16) = seal.nonce_sequential();
        subrange(packet, 24, 144) = box;

        return packet;
    }
};

int main(int argc, const char ** argv)
{
    kex_initiator client;
    kex_responder server;
    string msg;

    client.set_peer_pk(server.long_term_pk());

    try {
        msg = client.send_hello();
        hexdump(msg);
        msg = server.got_hello(msg);
        hexdump(msg);
        msg = client.got_cookie(msg);
        // hexdump(msg);
        server.got_initiate(msg);
        msg = client.send_message();
        // hexdump(msg);
        msg = server.send_message(msg);
        // hexdump(msg);
    } catch(const char* e) {
        cout << "Exception: " << e << endl;
    }
}
