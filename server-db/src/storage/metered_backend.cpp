//
// Created by benjamin-elias on 17.06.23.
//

#include "metered_backend.h"
#include "licensing/global_licensing.h"

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

void metered_backend::metered_alloc(std::size_t alloc)
{
    if (uh::dbn::licensing::global_license_pointer->license_package().has_soft_metred_feature(
        uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY))
    {
        if (!uh::dbn::licensing::global_license_pointer->license_package()
            .soft_limit_allocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                 alloc))
        {
            WARNING << "Storage backend \"" + backend_type() + "\" has only " +
                    std::to_string(uh::dbn::licensing::global_license_pointer->license_package()
                                       .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY))
                    + " bytes free on its license!";

            if (!uh::dbn::licensing::global_license_pointer->license_package()
                .hard_limit_allocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY,
                                     alloc))
            {
                std::string full_string = "Storage backend \"" + backend_type() + "\" is full. It has only " +
                    std::to_string(uh::dbn::licensing::global_license_pointer->license_package()
                                       .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY))
                    + " bytes free on its license and by that cannot store another chunk with "
                    + std::to_string(alloc) + " bytes!";

                ERROR << full_string;
                THROW(util::exception, full_string);
            }
        }
    }
}

// ---------------------------------------------------------------------

void metered_backend::metered_dealloc(std::size_t dealloc)
{
    uh::dbn::licensing::global_license_pointer->license_package()
        .deallocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY, dealloc);
}

// ---------------------------------------------------------------------

std::size_t metered_backend::metered_free_count()
{
    return uh::dbn::licensing::global_license_pointer->license_package()
        .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY);
}

// ---------------------------------------------------------------------

} // storage