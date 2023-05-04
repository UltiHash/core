#ifndef COMPRESSION_TYPE_H
#define COMPRESSION_TYPE_H

#include <io/device.h>

#include <cstdint>
#include <memory>


namespace uh::comp
{

// ---------------------------------------------------------------------

enum class type
{
    none,
    brotli
};

// ---------------------------------------------------------------------

type from_uint(std::uint32_t v);

// ---------------------------------------------------------------------

std::uint32_t to_uint(type t);

// ---------------------------------------------------------------------

std::string to_string(type t);

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, comp::type t);

// ---------------------------------------------------------------------

} // namespace uh::comp

#endif
