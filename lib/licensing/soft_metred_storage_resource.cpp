//
// Created by benjamin-elias on 01.06.23.
//

#include "soft_metred_storage_resource.h"

#include "util/exception.h"

namespace uh::licensing {

    // ---------------------------------------------------------------------

    bool soft_metred_storage_resource::hard_limit_allocate(std::size_t alloc) {
        bool out = stored_val + alloc <= hard_limit_val;
        if(out)stored_val += alloc;

        return out;
    }

    // ---------------------------------------------------------------------

    bool soft_metred_storage_resource::soft_limit_allocate(std::size_t alloc) {
        bool out = stored_val + alloc <= soft_limit_val;
        if(!hard_limit_allocate(alloc)) return false;

        return out;
    }

    // ---------------------------------------------------------------------

    void soft_metred_storage_resource::deallocate(std::size_t dealloc) {
        if(static_cast<long>(stored_val) < dealloc)
            THROW(util::exception, "License resource deallocation was called with underflow!");

        stored_val -= dealloc;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing