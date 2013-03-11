#pragma once

#include "byte_array.h"
#include "base32.h"

namespace ssu {

/**
 * Peer ID - helper for keeping all peer-related ID conversions in one place.
 * Contains a binary peer identifier plus methods to convert it into a string representation.
 */
class peer_id
{
    byte_array id_;

public:
    peer_id() : id_() {} 
    peer_id(std::string base32) : id_(encode::from_base32(base32)) {}
    peer_id(byte_array id) : id_(id) {}

    byte_array id() const { return id_; }
    std::string to_string() const { return encode::to_base32(id_); }
    bool is_empty() const { return id_.is_empty(); }
};

inline bool operator == (const peer_id& a, const peer_id& b) { return a.id() == b.id(); }
inline bool operator != (const peer_id& a, const peer_id& b) { return a.id() != b.id(); }

} // namespace ssu
