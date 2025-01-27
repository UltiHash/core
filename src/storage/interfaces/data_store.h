#pragma once

#include <common/types/address.h>
#include <span>

namespace uh::cluster {

struct data_store_config {
    size_t max_file_size;
    size_t max_data_store_size;
    size_t page_size;
};

struct data_store {
    virtual address write(std::span<const char> data,
                          const std::vector<std::size_t>& offsets) = 0;

    virtual void manual_write(uint64_t internal_pointer,
                              std::span<const char> data) = 0;

    virtual void manual_read(uint64_t pointer, std::span<char> buffer) = 0;
    virtual std::size_t read(const uint128_t& pointer,
                             std::span<char> buffer) = 0;

    virtual address link(const address& addr) = 0;

    virtual size_t unlink(const address& addr) = 0;

    virtual size_t get_used_space() const noexcept = 0;

    virtual size_t get_available_space() const noexcept = 0;

    virtual size_t id() const noexcept = 0;

    virtual ~data_store() = default;
};

} // end namespace uh::cluster
