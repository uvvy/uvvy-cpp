#pragma once

namespace filesyncbox {

/**
 * A chunk is a piece of data identified by a combination of hashes.
 *
 * A chunk is "positively matched" when all the hashes match search criteria.
 * A chunk is "partially matched" when at least one hash matches.
 */
class chunk
{
public:
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

