//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_SOFT_METRED_STORAGE_RESOURCE_H
#define CORE_SOFT_METRED_STORAGE_RESOURCE_H

#include <cstdio>
#include "licensing/soft_metered_resource.h"
#include "util/exception.h"

namespace uh::licensing
{

const std::string WARN_STORAGE_STRING = "warnStorage";
const std::string LIMIT_STORAGE_STRING = "limitStorage";

class soft_metered_storage_resource: public soft_metered_resource
{

public:
    /*
     * set up metred limits to check on the licensing model
     */
    soft_metered_storage_resource(std::size_t hard_limit, std::size_t soft_limit)
        :
        hard_limit_val(hard_limit), soft_limit_val(soft_limit)
    {
        if (hard_limit < soft_limit)
        THROW(util::exception, "The hard limit of a soft limited storage resource was smaller than it's soft"
                               "limit!");
    }

    /**
     * only allocate below warning limit, else do not allocate
     *
     * @param alloc space
     * @return decision false if no allocation could be acquired below warning level
     */
    [[nodiscard]] bool allocate(std::size_t alloc) override;

    /**
     *
     * @param dealloc
     */
    void deallocate(std::size_t dealloc) override;

    /**
     *
     * @return usable space limited by metred licensing
     */
    std::size_t free_count() override;

private:
    std::size_t stored_val{};
    const std::size_t hard_limit_val;
    const std::size_t soft_limit_val;
    bool warn_once = true;
};

} // namespace uh::licensing

#endif //CORE_SOFT_METRED_STORAGE_RESOURCE_H
