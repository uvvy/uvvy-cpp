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

const string helloPacketMagic     = "qVNq5xLh";
const string cookiePacketMagic    = "rl3Anmxk";
const string initiatePacketMagic  = "qVNq5xLi";
const string messagePacketMagic   = "rl3q5xLm";
const string helloNoncePrefix     = "cURVEcp-CLIENT-h";
const string minuteKeyNoncePrefix = "minute-k";
const string cookieNoncePrefix    = "cURVEcpk";
const string vouchNoncePrefix     = "cURVEcpv";
const string initiateNoncePrefix  = "cURVEcp-CLIENT-i";

// Initiator sends Hello and subsequently Initiate
class kex_initiator
{
    secret_key long_term_key;
    secret_key short_term_key;
    struct server {
        string long_term_key; // == kex_responder.long_term_key.pk
        string short_term_key;
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

    string got_cookie(string pkt)
    {
        assert(pkt.size() == 168);
        assert(subrange(pkt, 0, 8) == cookiePacketMagic);

        // open cookie box
        string nonce(24, '\0');
        subrange(nonce, 0, 8) = cookieNoncePrefix;
        subrange(nonce, 8, 16) = subrange(pkt, 8, 16);

        unboxer<recv_nonce> unseal(server.long_term_key, short_term_key, nonce);
        string open = unseal.unbox(subrange(pkt, 24, 144));

        server.short_term_key = subrange(open, 0, 32);
        string cookie = subrange(open, 32, 96);

        // @todo Must get payload from client
        return send_initiate(cookie, "Hello, world!");
    }

    string send_message() { return ""s; } // must be in client

private:
    string send_initiate(string cookie, string payload)
    {
        // Create vouch subpacket
        boxer<random_nonce<8>> vouchSeal(server.long_term_key, long_term_key, vouchNoncePrefix);
        string vouch = vouchSeal.box(short_term_key.pk.get());
        assert(vouch.size() == 48);

        // Assemble initiate packet
        string initiate(256+payload.size(), '\0');

        subrange(initiate, 0, 8) = initiatePacketMagic;
        subrange(initiate, 8, 32) = short_term_key.pk.get();
        subrange(initiate, 40, 96) = cookie;

        boxer<nonce64> seal(server.short_term_key, short_term_key, initiateNoncePrefix);
        subrange(initiate, 144, 112+payload.size())
            = seal.box(long_term_key.pk.get()+vouchSeal.nonce_sequential()+vouch+payload);
        // @todo Round payload size to next or second next multiple of 16..

        subrange(initiate, 136, 8) = seal.nonce_sequential();

        return initiate;
    }
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

    void got_initiate(string pkt) // end of negotiation
    {
        assert(subrange(pkt, 0, 8) == initiatePacketMagic);

        // Try to open the cookie
        string nonce(24, '\0');
        subrange(nonce, 0, 8) = minuteKeyNoncePrefix;
        subrange(nonce, 8, 16) = subrange(pkt, 40, 16);

        string cookie = crypto_secretbox_open(subrange(pkt, 56, 80), nonce, minute_key.get());

        // Check that cookie and client match
        assert(subrange(pkt, 8 ,32) == subrange(cookie, 0, 32));

        // Extract server short-term secret key
        short_term_key = secret_key(public_key(""), subrange(cookie, 32, 32));

        // Open the Initiate box using both short-term keys
        string initateNonce(24, '\0');
        subrange(initateNonce, 0, 16) = initiateNoncePrefix;
        subrange(initateNonce, 16, 8) = subrange(pkt, 136, 8);

        string clientKey = subrange(pkt, 8, 32);

        unboxer<recv_nonce> unseal(clientKey, short_term_key, initateNonce);
        string msg = unseal.unbox(subrange(pkt, 144));
        hexdump(msg);
    }

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
        hexdump(msg);
        server.got_initiate(msg);
        msg = client.send_message();
        // hexdump(msg);
        msg = server.send_message(msg);
        // hexdump(msg);
    } catch(const char* e) {
        cout << "Exception: " << e << endl;
    }
}
