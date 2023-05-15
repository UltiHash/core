//
// Created by masi on 5/15/23.
//
#include "mmap_storage.h"

class uh::dbn::storage::smart::mmap_storage::resource_entry {
public:
    resource_entry(void *p, size_t size):
            m_size (size),
            m_monotonic_buffer (p, size, std::pmr::null_memory_resource ()),
            m_pool_resource (&m_monotonic_buffer)
    {}

    std::pmr::memory_resource &get_pool_resource () {
        return m_pool_resource;
    }

    std::size_t get_size () const {
        return m_size;
    }

private:
    std::size_t m_size;
    std::pmr::monotonic_buffer_resource m_monotonic_buffer;
    std::pmr::synchronized_pool_resource m_pool_resource;

};

uh::dbn::storage::smart::mmap_storage::mmap_storage(const std::forward_list<file_mmap_info> &files):
    m_log (create_logger ()) {

    for (const auto &file: files) {
        mmap_file(file);
    }

    replay_logger();
}

void uh::dbn::storage::smart::mmap_storage::mmap_file(const file_mmap_info &file) {
    const auto fd = open(file.path.c_str(), O_CREAT | O_RDWR, S_IRWXU);
    ftruncate(fd, file.max_size);
    const auto flags = MAP_SHARED | MAP_FIXED | MAP_FIXED_NOREPLACE;
    const auto ptr = mmap(file.address, file.max_size, PROT_READ | PROT_WRITE, flags, fd, 0);
    close (fd);
    if (file.address != nullptr and ptr != file.address) {
        throw std::runtime_error("error: could not pin the file at the desired pointer!");
    }
    m_resources.emplace_hint(m_resources.cend(),
                             std::piecewise_construct,
                             std::forward_as_tuple (ptr),
                             std::forward_as_tuple (ptr, file.max_size));
}

std::fstream uh::dbn::storage::smart::mmap_storage::create_logger () {
    std::filesystem::path log_name = "_log_";
    auto flags = std::ios::in | std::ios::out;
    if (!std::filesystem::exists (log_name)) {
        flags |= std::ios::trunc;
    }
    return {log_name, flags};
}

void uh::dbn::storage::smart::mmap_storage::replay_logger() {
    std::string token;
    while (m_log >> token) {
        if (token == "alloc") {
            size_t bytes;
            m_log >> bytes;
            do_allocate (bytes);
        }
        else if (token == "dealloc") {
            size_t addr, bytes;
            m_log >> addr >> bytes;
            do_deallocate (reinterpret_cast <void *> (addr), bytes);
        }
        else {
            throw std::invalid_argument ("error: unknown token in the allocation log");
        }
    }
    m_log.clear ();
}

void *uh::dbn::storage::smart::mmap_storage::allocate(std::size_t size) {
    m_log << "alloc " << size << '\n';
    return do_allocate(size);
}

void uh::dbn::storage::smart::mmap_storage::deallocate(void *p, size_t size) {
    m_log << "dealloc " << p << ' ' << size << '\n';
    do_deallocate(p, size);
}

void uh::dbn::storage::smart::mmap_storage::sync(void *ptr, std::size_t size) {
    for (const auto &resource: m_resources) {
        msync (resource.first, resource.second.get_size(), MS_SYNC);
    }
}

void *uh::dbn::storage::smart::mmap_storage::do_allocate(size_t bytes) {
    for (auto &resource: m_resources) {
        try {
            return resource.second.get_pool_resource().allocate (bytes);
        }
        catch (std::bad_alloc) {

        }
    }
    throw std::bad_alloc ();
}

void uh::dbn::storage::smart::mmap_storage::do_deallocate(void *p, size_t bytes) {
    auto itr = m_resources.upper_bound(p);
    if (itr == m_resources.cbegin()) {
        throw std::domain_error ("error: deallocate request for non-existing resource");
    }
    itr --;
    if (static_cast<char *> (itr->first) + itr->second.get_size() < p) {
        throw std::domain_error ("error: deallocate request for non-existing resource");
    }
    itr->second.get_pool_resource().deallocate(p, bytes);
}
