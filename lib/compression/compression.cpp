#include "compression.h"

namespace uh::comp
{

// ---------------------------------------------------------------------

std::unique_ptr<io::device> create(std::unique_ptr<io::device>&& base, type t)
{
    switch (t)
    {
        case type::none: return io::owning_wrapper<none>(std::move(base));
        case type::brotli: return io::owning_wrapper<brotli>(std::move(base));
    }

    THROW(util::exception, "unsupported compression type: " + std::to_string(unsigned(t)));
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> create(io::device& base, type t)
{
    switch (t)
    {
        case type::none: return std::make_unique<none>(base);
        case type::brotli: return std::make_unique<brotli>(base);
    }

    THROW(util::exception, "unsupported compression type: " + std::to_string(unsigned(t)));
}

// ---------------------------------------------------------------------

} // namespace uh::comp
