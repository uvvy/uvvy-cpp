#pragma once

namespace filesyncbox {

/**
 * A chunk is a piece of data identified by a combination of hashes.
 * Typical chunk sizes are 256k, 512k, 1M, 2M, 4M.
 *
 * A chunk is "positively matched" when all the hashes match search criteria.
 * A chunk is "partially matched" when at least one hash matches.
 *
 * Hashes list from gentoo SHA256 SHA512 WHIRLPOOL size ;-)
 */
class chunk
{
    size_t size_factor_; // log2, e.g. 18 is 256k, 19 is 512k, 20 is 1M

public:
    size_t size() const {
        return 1 << size_factor_;
    }

    // outer_hash() const;
    // inner_hash() const;
    size_t small_hash() const;
};

} // filesyncbox namespace

// Hash specialization for chunk
namespace std {

template<>
struct hash<filesyncbox::chunk> : public std::unary_function<filesyncbox::chunk, size_t>
{
    inline size_t operator()(filesyncbox::chunk const& a) const noexcept
    {
        return a.small_hash();
    }
};

} // namespace std

