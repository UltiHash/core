//
// Created by benjamin-elias on 01.06.23.
//

#include "soft_metered_storage_resource.h"

#include "util/exception.h"
#include "logging/logging_boost.h"

namespace uh::licensing
{

// ---------------------------------------------------------------------

bool soft_metered_storage_resource::allocate(std::size_t alloc)
{
    if (stored_val + alloc <= soft_limit_val)
    {
        stored_val += alloc;
        warn_once = true;
        return true;
    }

    if (stored_val + alloc <= hard_limit_val)
    {
        stored_val += alloc;
        if (warn_once)
        {
            WARNING
                << "Soft metered storage resource reached warning limit with " + std::to_string(stored_val) + " from "
                    + std::to_string(hard_limit_val) + " bytes.";
            warn_once = false;
        }

        return true;
    }

    THROW(util::exception, "Out of licensed storage!");
}

// ---------------------------------------------------------------------

void soft_metered_storage_resource::deallocate(std::size_t dealloc)
{
    if (static_cast<long>(stored_val) < dealloc)
    THROW(util::exception, "License resource deallocation was called with underflow!");

    stored_val -= dealloc;
}

// ---------------------------------------------------------------------

std::size_t soft_metered_storage_resource::free_count()
{
    return hard_limit_val - stored_val;
}

// ---------------------------------------------------------------------

} // namespace uh::licensing