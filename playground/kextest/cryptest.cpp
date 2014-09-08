#include <sodiumpp/sodiumpp.h>
#include <string>
#include <iostream>
#include "arsenal/proquint.h"
#include "arsenal/base32x.h"
#include "arsenal/base64.h"

using namespace sodiumpp;
using namespace std;

// Variants of the same key: (show all of these to the user? I'd stick with QR and proquint)
//
// As QR code:
// http://qrickit.com/api/qr?d=%77%2f%CC%C3%5b%46%41%64%FC%EC%72%65%D0%C0%27%CA%5c%96%92%5e%BF%76%D5%85%13%A1%98%96%27%E3%CC%51
// As base32x with groups:
// Q6Z6 3S45 J3AY K9HN QKU7 BSBH 3KQK PEU8 Z75P MBJV WGNK NK9D 3TJS
// As proquint:
// lisoz-sozag-jotak-hajoh-zozos-lanoj-suzab-firap-jovik-naniv-rutuk-tifaj-dapod-nivik-firog-sudid
// As base64:
// dy/Mw1tGQWT87HJl0MAnylyWkl6/dtWFE6GYlifjzFE
// As hex:
// 772fccc35b464164fcec7265d0c027ca5c96925ebf76d58513a1989627e3cc51

std::ostream& print_base32x_key(std::ostream& stream, const sodiumpp::public_key& pk) {
    return stream << "public_key(\"" << encode::to_base32x(pk.get()) << "\")";
}

std::ostream& print_base64_key(std::ostream& stream, const sodiumpp::public_key& pk) {
    return stream << "public_key(\"" << encode::to_base64(pk.get()) << "\")";
}

std::ostream& print_proquint_key(std::ostream& stream, const sodiumpp::public_key& pk) {
    return stream << "public_key(\"" << encode::to_proquint(pk.get()) << "\")";
}

struct hello_packet
{
    string short_term_client;
    string client_short_nonce;
    string box;
};

struct cookie_packet
{
    string server_short_nonce;
    string box;
};

struct initiate_packet
{
    string short_term_client;
    string client_short_nonce;
    string box;
};

int main(int argc, const char ** argv)
{
    secret_key long_term_client;
    secret_key long_term_server; // = host_identity()
    secret_key short_term_client;
    secret_key short_term_server; // = host.shortterm_key
    secret_key minute_key; // to construct and verify cookies

    // There's a page describing how curvecp nonces should be generated: http://curvecp.org/nonces.html

    // Create a nonce type that has a 64-bit sequential counter and constant random bytes for the remaining bytes
    typedef nonce<crypto_box_NONCEBYTES-8, 8> nonce64;

    // Server nonce
    typedef nonce<crypto_box_NONCEBYTES-16, 16> nonce128;

    // Client prepares and sends Hello packet:
    hello_packet hello;
    hello.short_term_client = short_term_client.pk.get();
    boxer<nonce64> client_hello_boxer(long_term_server.pk, short_term_client, "CurveCP-client-H");
    hello.client_short_nonce = client_hello_boxer.nonce_sequential();
    hello.box = client_hello_boxer.box(string(64, '\0'));

    // Now server receives Hello and tries to unbox it:
    unboxer<nonce64> server_hello_unboxer(hello.short_term_client, long_term_server, "CurveCP-client-H");
    string result = server_hello_unboxer.unbox(hello.box);

    assert(result == string(64, '\0'));

    // Server prepares Cookie packet:
    cookie_packet cookie;
    boxer<nonce128> server_cookie_boxer(hello.short_term_client, long_term_server, "CurveCPK");
    cookie.server_short_nonce = server_cookie_boxer.nonce_sequential();
    cookie.box = server_cookie_boxer.box(short_term_server.pk.get());

    // Client receives Cookie packet and gets server short-term key for session.
    unboxer<nonce128> client_cookie_unboxer(short_term_client.pk, long_term_server, "CurveCPK");
    string rcv_st_server = client_cookie_unboxer.unbox(cookie.box);

    // Client prepares and sends Initiate packet:

    // Box with client private key for server public key to unbox.
    // boxer<nonce64> client_boxer(sk_server.pk, sk_client);
    // Unbox with server private key and passed client nonce.
    // unboxer<nonce64> server_unboxer(sk_client.pk, sk_server, client_boxer.nonce_constant());

    // string boxed = client_boxer.box("Hello, world!\n");
    // cout << bin2hex(boxed) << endl;
    // string unboxed = server_unboxer.unbox(boxed);
    // cout << unboxed;

    // Box with server private key for client public key to unbox.
    // boxer<nonce64> server_boxer(sk_client.pk, sk_server);
    // Unbox with client private key and passed server nonce.
    // unboxer<nonce64> client_unboxer(sk_server.pk, sk_client, server_boxer.nonce_constant());

    // boxed = server_boxer.box("From sodiumpp!\n");
    // cout << bin2hex(boxed) << endl;
    // unboxed = client_unboxer.unbox(boxed);
    // cout << unboxed;

    // TODO: Simulate a curvecp-like exchange
    // Hello

    // Cookie
    // Initiate
    // Message

    return 0;
}
