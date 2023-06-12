//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_SOFT_METRED_STORAGE_RESOURCE_H
#define CORE_SOFT_METRED_STORAGE_RESOURCE_H

#include <cstdio>
#include "licensing/soft_metred_resource.h"
#include "util/exception.h"

namespace uh::licensing {

    class soft_metred_storage_resource: public soft_metred_resource{

    public:
        /*
         * set up metred limits to check on the licensing model
         */
        soft_metred_storage_resource(std::size_t hard_limit, std::size_t soft_limit):
        hard_limit_val(hard_limit), soft_limit_val(soft_limit){
            if(hard_limit < soft_limit)
                THROW(util::exception,"The hard limit of a soft limited storage resource was smaller than it's soft"
                                      "limit!");
        }

        /**
         * check if the resource should be blocked when trying to allocate an amount of space
         *
         * @param alloc space
         * @return decision
         */
        [[nodiscard]] bool hard_limit_allocate(std::size_t alloc) override;

        /**
         * check if the resource should be warned about when trying to allocate an amount of space
         *
         * @param alloc space
         * @return decision
         */
        [[nodiscard]] bool soft_limit_allocate(std::size_t alloc) override;

        /**
         *
         * @param dealloc
         */
        void deallocate(std::size_t dealloc) override;

    private:
        std::size_t stored_val{};
        const std::size_t hard_limit_val;
        const std::size_t soft_limit_val;
    };

} // namespace uh::licensing

#endif //CORE_SOFT_METRED_STORAGE_RESOURCE_H
