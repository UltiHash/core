#pragma once

#include <common/types/address.h>
#include <common/types/common_types.h>
#include <common/utils/common.h>
#include <span>

namespace uh::cluster {

struct data_store_config {
    size_t max_file_size = 1_GiB;
    size_t max_data_store_size = 1_PiB;
    size_t page_size = DEFAULT_PAGE_SIZE;
};

struct data_store {
    virtual allocation_t allocate(size_t size, std::size_t alignment = 1) = 0;

    virtual address write(const allocation_t allocation,
                          const std::vector<std::span<const char>>& buffers,
                          const std::vector<refcount_t>& refcounts = {}) = 0;

    virtual std::size_t read(const std::size_t local_pointer,
                             std::span<char> buffer) = 0;

    virtual std::vector<refcount_t>
    link(const std::vector<refcount_t>& refcounts) = 0;

    virtual std::size_t unlink(const std::vector<refcount_t>& refcounts) = 0;

    virtual std::vector<refcount_t>
    get_refcounts(const std::vector<std::size_t>& stripe_ids) = 0;

    virtual std::size_t get_used_space() const noexcept = 0;

    virtual std::size_t get_available_space() const noexcept = 0;

    virtual ~data_store() = default;
};

} // end namespace uh::cluster
