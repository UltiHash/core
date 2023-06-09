//
// Created by masi on 5/15/23.
//

#ifndef CORE_FIXED_MANAGED_STORAGE_H
#define CORE_FIXED_MANAGED_STORAGE_H

#include "smart_config.h"

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <memory_resource>
#include <map>
#include <set>
#include <system_error>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <boost/interprocess/mapped_region.hpp>

namespace uh::dbn::storage::smart {

class fixed_managed_storage;
class growing_managed_storage;

void* align_ptr (void* ptr);

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
    std::pmr::synchronized_pool_resource m_pool_resource;
};

class fixed_managed_storage {

public:

    explicit fixed_managed_storage (data_store_config);

    /** Allocate new memory in the mmap_storage and return a pointer to it.
     *  @throws bad_alloc
     */
    offset_ptr allocate (std::size_t size);

    /** Deallocate the memory pointed by p for size number of bytes.
     *  @throws bad_alloc
     */
    void deallocate (const offset_ptr&, size_t size);

    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    void sync (void* ptr, std::size_t size);

    /**
     * Flushes the whole mmap storage to disk. Only return when sync was finished.
     */
    void sync ();

    /**
     * Transforms the given offset to a pointer on the memory.
     * @param offset
     * @return pointer
     */
    void* get_raw_ptr (size_t offset);

    ~fixed_managed_storage();

private:



    offset_ptr do_allocate (size_t bytes);

    void do_deallocate (const offset_ptr& , size_t bytes);

    void mmap_file (const std::filesystem::path& file);

    resource_entry& get_resource (size_t offset, size_t size = 0);

    std::fstream create_logger () const;

    void replay_logger ();

    std::filesystem::path generate_log_file_path () ;

    bool files_existence_consistency ();

    const data_store_config m_conf;
    std::filesystem::path m_log_file_path;
    std::fstream m_log;
    std::map <size_t, resource_entry> m_resources;
    std::size_t m_aggregated_size {};
};


} // end namespace uh::dbn::storage::smart

#endif //CORE_FIXED_MANAGED_STORAGE_H
