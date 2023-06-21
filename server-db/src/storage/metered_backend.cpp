//
// Created by benjamin-elias on 19.06.23.
//

#include "metered_backend.h"

namespace uh::dbn::storage
{

void metered_backend::metered_alloc(std::size_t alloc)
{
    uh::dbn::licensing::global_license_pointer_dbn->license_package()
        .allocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY, alloc);
}

// ---------------------------------------------------------------------

void metered_backend::metered_dealloc(std::size_t dealloc)
{
    uh::dbn::licensing::global_license_pointer_dbn->license_package()
        .deallocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY, dealloc);
}

// ---------------------------------------------------------------------

std::size_t metered_backend::metered_free_count()
{
    return uh::dbn::licensing::global_license_pointer_dbn->license_package()
        .free_count(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY);
}

} // namespace uh::dbn::storage