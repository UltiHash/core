#include "type.h"

#include <util/exception.h>


namespace uh::comp
{

// ---------------------------------------------------------------------

type from_uint(std::uint32_t v)
{
    switch (v)
    {
        case 0x00: return type::none;
    }

    THROW(util::exception, "unsupported compression id");
}

// ---------------------------------------------------------------------

std::uint32_t to_uint(type t)
{
    switch (t)
    {
        case type::none: return 0x00;
    }

    THROW(util::exception, "unsupported compression type");
}

// ---------------------------------------------------------------------

std::string to_string(type t)
{
    switch (t)
    {
        case type::none: return "none";
    }

    THROW(util::exception, "unsupported compression type");
}

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, comp::type t)
{
    out << to_string(t);
    return out;
}

// ---------------------------------------------------------------------

} // namespace uh::comp
