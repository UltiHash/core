#ifndef PROTOCOL_ALLOCATION_H
#define PROTOCOL_ALLOCATION_H

#include <io/device.h>
#include "common.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

/**
 * Interface for memory allocation: the allocated memory is reserved for the
 * lifetime of this object. When the object is destructed the reserved memory
 * is freed *unless* `persist` was called on the object.
 */
class allocation
{
public:
    virtual ~allocation() = default;

    /**
     * Return a reference to the underlying device, allowing direct write
     * to allocated memory.
     */
    virtual io::device& device() = 0;

    /**
     * Make the allocation persistent and return a hash code for the written
     * block.
     */
    virtual block_meta_data persist() = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
