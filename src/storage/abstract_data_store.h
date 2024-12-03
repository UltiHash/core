#pragma once

#include "common/types/address.h"
#include <string_view>

namespace uh::cluster {

struct abstract_data_store {
    virtual address write(const std::string_view& data);

    virtual void manual_write(uint64_t internal_pointer,
                              const std::string_view& data);

    virtual void manual_read(uint64_t pointer, size_t size, char* buffer);

    virtual std::size_t read(char* buffer, const uint128_t& pointer,
                             size_t size);
    virtual std::size_t read_up_to(char* buffer, const uint128_t& pointer,
                                   size_t size);

    virtual address link(const address& addr);

    virtual size_t unlink(const address& addr);

    virtual size_t get_used_space() const noexcept;

    virtual size_t get_available_space() const noexcept;

    virtual size_t id() const noexcept;

    virtual ~abstract_data_store() = default;
};

} // end namespace uh::cluster
