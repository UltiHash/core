#pragma once

#include <storage/interface.h>

namespace uh::cluster {

class distributed_storage : public sn::interface {
public:
    /**
     * Read from the storage service to specified regions of the buffer.
     */
    virtual coro<void> read_address(context& ctx, const address& addr,
                                    std::span<char> buffer,
                                    const std::vector<size_t>& offsets) = 0;
};

} // namespace uh::cluster
