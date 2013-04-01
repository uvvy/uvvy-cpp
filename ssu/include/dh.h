#pragma once

#include "byte_array.h"
#include "negotiation/key_message.h"

class dh_hostkey_t
{
public:
    byte_array public_key;
    byte_array hkr; // hmac secret key
};

class dh_host_state
{
public:
    dh_hostkey_t* get_dh_key(ssu::negotiation::dh_group_type group) { return 0; }
};
