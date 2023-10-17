#ifndef PROTOCOL_HASH_H
#define PROTOCOL_HASH_H

#include <array>


namespace uh::protocol
{

// ---------------------------------------------------------------------

typedef std::array<char, 64> hash;

// ---------------------------------------------------------------------

constexpr std::size_t hash_size()
{
    return std::tuple_size<hash>{};
}

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
