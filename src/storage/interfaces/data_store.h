#pragma once

#include <common/types/address.h>
#include <common/types/common_types.h>
#include <span>

namespace uh::cluster {

struct data_store_config {
    size_t max_file_size;
    size_t max_data_store_size;
    size_t page_size;
};

struct data_store {
    virtual allocation_t allocate(size_t size, std::size_t alignment = 1) = 0;

    virtual address write(const allocation_t allocation,
                          std::span<const char> data,
                          const std::vector<std::size_t>& offsets) = 0;

    virtual std::size_t read(const std::size_t local_pointer,
                             std::span<char> buffer) = 0;

    virtual address link(const address& addr) = 0;

    virtual std::size_t unlink(const address& addr) = 0;

    virtual std::size_t get_used_space() const noexcept = 0;

    virtual std::size_t get_available_space() const noexcept = 0;

    virtual ~data_store() = default;
};

} // end namespace uh::cluster
