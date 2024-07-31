//
// Created by massi on 7/31/24.
//

#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H
#include "storage_interface.h"

namespace uh::cluster {

class storage_system : public storage_interface {
    virtual coro<address> write(context& ctx, const std::string_view&) = 0;
    virtual coro<void> read_fragment(context& ctx, char* buffer,
                                     const fragment& f) = 0;
    virtual coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                                       size_t size) = 0;
    virtual coro<void> read_address(context& ctx, char* buffer,
                                    const address& addr,

                                    const std::vector<size_t>& offsets) = 0;

    virtual coro<void> link(context& ctx, const address& addr) = 0;
    virtual coro<void> unlink(context& ctx, const address& addr) = 0;
    virtual coro<void> sync(context& ctx, const address& addr) = 0;
    virtual coro<size_t> get_used_space(context& ctx) = 0;
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
