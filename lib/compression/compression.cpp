#include "compression.h"

#include <util/exception.h>
#include <compression/none.h>


namespace uh::comp
{

// ---------------------------------------------------------------------

std::unique_ptr<io::device> create(std::unique_ptr<io::device>&& base, type t)
{
    switch (t)
    {
        case type::none: return io::owning_wrapper<none>(std::move(base));
    }

    THROW(util::exception, "unsupported compression type: " + std::to_string(unsigned(t)));
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> create(io::device& base, type t)
{
    switch (t)
    {
        case type::none: return std::make_unique<none>(base);
    }

    THROW(util::exception, "unsupported compression type: " + std::to_string(unsigned(t)));
}

// ---------------------------------------------------------------------

} // namespace uh::comp
