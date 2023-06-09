//
// Created by masi on 6/8/23.
//

#ifndef CORE_STORAGE_COMMON_H
#define CORE_STORAGE_COMMON_H

#include <filesystem>
#include <fstream>
#include <map>
#include <memory_resource>
#include <sys/mman.h>

namespace uh::dbn::storage::smart {

class fixed_managed_storage;
class growing_managed_storage;

void* align_ptr (void* ptr) noexcept;
void sync_ptr (void *ptr, std::size_t size);

class offset_ptr {
public:
    offset_ptr (size_t offset = 0, void* addr = nullptr);
    size_t m_offset;
    char* m_addr;

    bool operator == (const offset_ptr& ptr) const noexcept {
        return m_addr == ptr.m_addr;
    }
private:
    [[nodiscard]] offset_ptr get_offset_ptr_at (size_t offset) const;
    [[nodiscard]] offset_ptr get_offset_ptr_at (void* raw_ptr) const;

    friend fixed_managed_storage;
    friend growing_managed_storage;

};

class resource_entry {
public:
    resource_entry (void* addr, std::filesystem::path path, size_t size, size_t offset);
    std::filesystem::path m_path;
    std::pmr::memory_resource &get_pool_resource();
    offset_ptr m_ptr;
    const size_t m_size;
private:
    std::pmr::monotonic_buffer_resource m_monotonic_buffer;
    std::pmr::unsynchronized_pool_resource m_pool_resource;
};


class managed_storage {

protected:
    managed_storage () = default;

    void mmap_file (const std::filesystem::path& file, uint64_t offset, size_t file_size);

    resource_entry& get_resource (size_t offset, size_t size = 0);

    std::map <size_t, resource_entry> m_resources;
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_STORAGE_COMMON_H
