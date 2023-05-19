//
// Created by masi on 5/15/23.
//

#ifndef CORE_MMAP_STORAGE_H
#define CORE_MMAP_STORAGE_H

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <memory_resource>
#include <map>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>

namespace uh::dbn::storage::smart {


struct file_mmap_info {
    std::filesystem::path path;
    void *address;
    std::size_t max_size;
};

class offset_ptr {
public:

    offset_ptr (size_t offset = 0, void* addr = nullptr):
        m_addr (static_cast <char*> (addr)), m_offset (offset) {}

    size_t m_offset;
    char* m_addr;

    [[nodiscard]] offset_ptr get_offset_ptr_at (size_t offset) const {
        if (m_addr == nullptr) {
            throw std::logic_error ("error: nullptr in offset_ptr");
        }
        return {offset, (offset - m_offset) + static_cast <char*> (m_addr)};
    }

    [[nodiscard]] offset_ptr get_offset_ptr_at (void* raw_ptr) const {
        if (m_addr == nullptr) {
            throw std::logic_error ("error: nullptr in offset_ptr");
        }
        return {(static_cast <char*> (raw_ptr) - static_cast <char*> (m_addr)) + m_offset, raw_ptr};
    }
};

class resource_entry {
public:
    resource_entry (void* addr, size_t size, size_t offset):
            m_ptr (offset, addr),
            m_size (size),
            m_monotonic_buffer(addr, size, std::pmr::null_memory_resource()),
            m_pool_resource(&m_monotonic_buffer) {}

    std::pmr::memory_resource &get_pool_resource() {
        return m_pool_resource;
    }
    offset_ptr m_ptr;
    const size_t m_size;
private:
    std::pmr::monotonic_buffer_resource m_monotonic_buffer;
    std::pmr::synchronized_pool_resource m_pool_resource;
};

class resource_container {


public:

    void add_resource (const std::filesystem::path& path, void* addr, size_t size) {
        const auto res = m_resource_paths.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(path),
                                                  std::forward_as_tuple(addr, size, m_aggregate_size));
        m_resource_offsets.emplace_hint(m_resource_offsets.cend(), res.first->second.m_ptr.m_offset, res.first->second);
        m_aggregate_size += size;
    }

    resource_entry& get_resource (size_t offset, size_t size = 0) {
        auto itr = m_resource_offsets.upper_bound (offset);
        if (itr == m_resource_offsets.cbegin()) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        itr--;
        if (itr->first + itr->second.m_size < offset + size) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        return itr->second;
    }

    std::unordered_map <std::filesystem::path, resource_entry>& get_resources () {
        return m_resource_paths;
    }

private:

    std::unordered_map <std::filesystem::path, resource_entry> m_resource_paths;
    std::map <size_t, resource_entry&> m_resource_offsets;
    std::size_t m_aggregate_size {};

};

class mmap_storage {

public:

    explicit mmap_storage (const std::forward_list<file_mmap_info>& files);

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

    void* get_raw_ptr (size_t offset);

private:

    offset_ptr do_allocate (size_t bytes);

    void do_deallocate (const offset_ptr& , size_t bytes);

    void mmap_file (const file_mmap_info& file);

    std::fstream create_logger () const;

    void replay_logger ();

    static std::filesystem::path generate_log_file_path (const std::forward_list<file_mmap_info>& files) ;

    static bool files_consistent_existency (const std::forward_list<file_mmap_info>& files);

    std::filesystem::path m_log_file_path;
    std::fstream m_log;
    resource_container m_resource_container;

};


} // end namespace uh::dbn::storage::smart

#endif //CORE_MMAP_STORAGE_H
