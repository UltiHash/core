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
    offset_ptr () = default;
    offset_ptr (uint64_t offset, void* addr = nullptr);
    uint64_t m_offset {};
    char* m_addr = nullptr;

    bool operator == (const offset_ptr& ptr) const noexcept;
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
public:

    /** Allocate new memory in the mmap_storage and return a pointer to it.
  *  @throws bad_alloc
  */
    virtual offset_ptr allocate (std::size_t size) = 0;

    /** Deallocate the memory pointed by p for size number of bytes.
     *  @throws bad_alloc
     */
    virtual void deallocate (const offset_ptr&, size_t size) = 0;

    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    virtual void sync (void* ptr, std::size_t size) = 0;

    /**
     * Flushes the whole mmap storage to disk. Only return when sync was finished.
     */
    virtual void sync ();

    /**
     * Transforms the given offset to a pointer on the memory.
     * @param offset
     * @return pointer
     */
    virtual void* get_raw_ptr (size_t offset) = 0;

protected:
    managed_storage () = default;

    void mmap_file (const std::filesystem::path& file, uint64_t offset, size_t file_size);

    resource_entry& get_resource (size_t offset, size_t size = 0);

    std::map <size_t, resource_entry> m_resources;
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_STORAGE_COMMON_H
