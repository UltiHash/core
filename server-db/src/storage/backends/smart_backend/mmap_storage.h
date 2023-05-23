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

class mmap_storage;

struct file_mmap_info {
    std::filesystem::path path;
    void *address;
    std::size_t max_size;
};

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
    friend mmap_storage;
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

    /**
     * Transforms the given offset to a pointer on the memory.
     * @param offset
     * @return pointer
     */
    void* get_raw_ptr (size_t offset);

private:

    class resource_entry {
    public:
        resource_entry (void* addr, size_t size, size_t offset);

        std::pmr::memory_resource &get_pool_resource();
        offset_ptr m_ptr;
        const size_t m_size;
    private:
        std::pmr::monotonic_buffer_resource m_monotonic_buffer;
        std::pmr::synchronized_pool_resource m_pool_resource;
    };

    offset_ptr do_allocate (size_t bytes);

    void do_deallocate (const offset_ptr& , size_t bytes);

    void mmap_file (const file_mmap_info& file);

    resource_entry& get_resource (size_t offset, size_t size = 0);

    std::fstream create_logger () const;

    void replay_logger ();

    static std::filesystem::path generate_log_file_path (const std::forward_list<file_mmap_info>& files) ;

    static bool files_consistent_existency (const std::forward_list<file_mmap_info>& files);

    std::filesystem::path m_log_file_path;
    std::fstream m_log;
    std::map <size_t, resource_entry> m_resources;
    std::size_t m_aggregated_size {};

};


} // end namespace uh::dbn::storage::smart

#endif //CORE_MMAP_STORAGE_H
