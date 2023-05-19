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

namespace uh::dbn::storage::smart {


struct file_mmap_info {
    std::filesystem::path path;
    void *address;
    std::size_t max_size;
};

class resource_container {
    class resource_entry {
    public:
        resource_entry (void *addr, size_t size):
                m_addr (addr),
                m_size (size),
        m_monotonic_buffer(addr, size, std::pmr::null_memory_resource()),
        m_pool_resource(&m_monotonic_buffer) {}

        std::pmr::memory_resource &get_pool_resource() {
            return m_pool_resource;
        }
        void* const m_addr;
        const size_t m_size;
        void* m_init = nullptr;
    private:
        std::pmr::monotonic_buffer_resource m_monotonic_buffer;
        std::pmr::synchronized_pool_resource m_pool_resource;
    };

public:

    struct Iterator
    {
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::pmr::memory_resource;
        using pointer           = value_type*;
        using reference         = value_type&;

        explicit Iterator(std::unordered_map <std::filesystem::path, resource_entry>::iterator begin):
            m_itr (begin) {}

        reference operator*() const { return m_itr->second.get_pool_resource ();}
        pointer operator->() { return  &m_itr->second.get_pool_resource (); }
        Iterator& operator++() { m_itr++; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }
        friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_itr == b.m_itr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_itr != b.m_itr; };

    private:
        std::unordered_map <std::filesystem::path, resource_entry>::iterator m_itr;

    };

    void add_resource (const std::filesystem::path& path, void* addr, size_t size) {
        const auto res = m_resource_paths.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(path),
                                                  std::forward_as_tuple(addr, size));
        m_resource_current_addresses.emplace_hint(m_resource_current_addresses.cend(), addr, res.first->second);
    }

    std::pmr::memory_resource& get_resource (void* ptr, size_t size) {
        auto itr = m_resource_current_addresses.upper_bound (ptr);
        if (itr == m_resource_current_addresses.cbegin()) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        itr--;
        if (static_cast <char* const> (itr->second.m_addr) + itr->second.m_size < ptr) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        return itr->second.get_pool_resource();
    }

    void* get_current_ptr (void* init_ptr) {
        auto itr = m_resource_init_addresses.upper_bound (init_ptr);
        if (itr == m_resource_init_addresses.cbegin()) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        itr--;
        const auto init_addr = itr->first;
        const auto current_addr = itr->second.m_addr;
        return static_cast <char*> (current_addr) + (static_cast <char*> (init_ptr) - static_cast <char*> (init_addr));
    }

    void* get_init_ptr (void* current_ptr) {
        auto itr = m_resource_current_addresses.upper_bound (current_ptr);
        if (itr == m_resource_current_addresses.cbegin()) {
            throw std::domain_error("error: deallocate request for non-existing resource");
        }
        itr--;
        const auto current_addr = itr->first;
        const auto init_addr = itr->second.m_init;
        return static_cast <char*> (init_addr) + (static_cast <char*> (current_ptr) - static_cast <char*> (current_addr));
    }

    void set_init_address (const std::filesystem::path& path, void* init_addr) {
        auto &resource = m_resource_paths.at(path);
        resource.m_init = init_addr;
        m_resource_init_addresses.emplace_hint(m_resource_init_addresses.cend(), init_addr, resource);
    }

    Iterator begin () { return Iterator(m_resource_paths.begin()); }
    Iterator end () { return Iterator(m_resource_paths.end()); }

private:

    std::unordered_map <std::filesystem::path, resource_entry> m_resource_paths;
    std::map <void* const, resource_entry&> m_resource_init_addresses;
    std::map <void* const, resource_entry&> m_resource_current_addresses;


};

class mmap_storage {

public:

    explicit mmap_storage (const std::forward_list<file_mmap_info> &files);

    /** Allocate new memory in the mmap_storage and return a pointer to it.
     *  @throws bad_alloc
     */
    void* allocate (std::size_t size);

    /** Deallocate the memory pointed by p for size number of bytes.
     *  @throws bad_alloc
     */
    void deallocate (void *p, size_t size);

    void *get_offset_ptr (void* ptr) {
        auto &resource = m_resource_container.get_resource (p, bytes);

        std::size_t offset_ptr = 0;
        for (auto &resource: m_resource_container) {
            try {
                offset_ptr += resource.allocate (bytes) - resource.offset;
                return reinterpret_cast <void*> (offset_ptr);
            }
            catch (std::bad_alloc &) {
                offset_ptr += resource.size;
            }
        }
        throw std::bad_alloc();
    }


    /** Flush changes to the memory to disk. Only return when sync was finished.
     *  @throws on error
     */
    void sync (void *ptr, std::size_t size);

private:

    void* do_allocate (size_t bytes);

    void do_deallocate (void *p, size_t bytes);

    void mmap_file (const file_mmap_info &file);

    std::fstream create_logger () const;

    void replay_logger ();

    static std::filesystem::path generate_log_file_path (const std::forward_list<file_mmap_info> &files) ;

    static bool files_consistent_existency (const std::forward_list<file_mmap_info> &files);

    bool m_init_run;
    std::filesystem::path m_log_file_path;
    std::fstream m_log;
    resource_container m_resource_container;

};


} // end namespace uh::dbn::storage::smart

#endif //CORE_MMAP_STORAGE_H
