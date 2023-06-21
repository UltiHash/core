//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_SOFT_METRED_RESOURCE_H
#define CORE_SOFT_METRED_RESOURCE_H

#include <cstddef>

namespace uh::licensing
{

class soft_metered_resource
{
public:

    ~soft_metered_resource() = default;

    /**
     *
     * @param dealloc a resource
     */
    virtual void deallocate(std::size_t dealloc) = 0;

    /**
     *
     * @return count of free metred elements
     */
    virtual std::size_t free_count() = 0;

    /**
     *
     * @return if a warning level limit of the implemented resource has been reached
     */
    virtual bool allocate(std::size_t alloc) = 0;

};

}

#endif //CORE_SOFT_METRED_RESOURCE_H
