//
// Created by benjamin-elias on 17.06.23.
//

#include "licensed_backend.h"

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

void licensed_backend::licensed_alloc(std::size_t alloc)
{
    if (licensing_global_module->license_package().has_soft_metred_feature(
        uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY))
    {
        if (!licensing_global_module->license_package()
            .soft_limit_allocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY,
                                 alloc))
        {
            WARNING << "Storage backend \"" + backend_type() + "\" has only " +
                    std::to_string(licensing_global_module->license_package()
                                       .free_count(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY))
                    + " bytes free on its license!";

            if (!licensing_global_module->license_package()
                .hard_limit_allocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY,
                                     alloc))
            {
                std::string full_string = "Storage backend \"" + backend_type() + "\" is full. It has only " +
                    std::to_string(licensing_global_module->license_package()
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

void licensed_backend::licensed_dealloc(std::size_t dealloc)
{
    licensing_global_module->license_package()
        .deallocate(uh::licensing::license_package::hard_metered_feature::LIMIT_STORAGE_CAPACITY, dealloc);
}

// ---------------------------------------------------------------------

} // storage