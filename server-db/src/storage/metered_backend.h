//
// Created by benjamin-elias on 17.06.23.
//

#ifndef CORE_LICENSED_BACKEND_H
#define CORE_LICENSED_BACKEND_H

#include "storage/backend.h"
#include "licensing/global_licensing.h"

namespace uh::dbn::storage
{

class metered_backend: public backend
{
public:
    /**
     * Checks and alloces the available license storage space
     * Writes warning to log file in case the warning limit was surpassed
     * @throw if license limit would be exceeded by storing the data --> allocation fails
     */
    virtual void metered_alloc(std::size_t alloc);

    /**
     *
     * @param dealloc how much space should be deallocated from the license counter
     * @throw in case someone tries to deallocate below zero
     */
    virtual void metered_dealloc(std::size_t dealloc);

    /**
     *
     * @return free space due to licensing
     */
    virtual std::size_t metered_free_count();

};

} // namespace uh::dbn::storage

#endif //CORE_LICENSED_BACKEND_H
