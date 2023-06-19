//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_SOFT_METRED_RESOURCE_H
#define CORE_SOFT_METRED_RESOURCE_H

#include "licensing/metred_resource.h"
#include <cstddef>

namespace uh::licensing
{

class soft_metered_resource: public metred_resource
{

public:

    /**
     *
     * @return if a warning level limit of the implemented resource has been reached
     */
    virtual bool soft_limit_allocate(std::size_t alloc) = 0;

};

}

#endif //CORE_SOFT_METRED_RESOURCE_H
