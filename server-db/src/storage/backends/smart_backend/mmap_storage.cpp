//
// Created by masi on 5/15/23.
//
#include <unordered_map>
#include "mmap_storage.h"

namespace uh::dbn::storage::smart {



mmap_storage::mmap_storage(const std::forward_list<file_mmap_info>& files):
    m_log_file_path (generate_log_file_path(files)),
    m_log(create_logger()) {

    files_consistent_existency(files);
    for (const auto &file: files) {
        mmap_file (file);
    }

    replay_logger();
}

offset_ptr mmap_storage::allocate(std::size_t size) {
    m_log << "al " << size << '\n';
    return do_allocate(size);
}

void mmap_storage::deallocate(const offset_ptr& off_ptr, size_t size) {
    m_log << "de " << off_ptr.m_offset << ' ' << size << '\n';
    do_deallocate(off_ptr, size);
}

void mmap_storage::sync(void *ptr, std::size_t size) {
    msync(ptr, size, MS_SYNC);
}

void *mmap_storage::get_raw_ptr(size_t offset) {
    return get_resource(offset).m_ptr.get_offset_ptr_at(offset).m_addr;
}

void mmap_storage::mmap_file(const file_mmap_info& file) {

    const auto fd = open(file.path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(fd, file.max_size);
    auto flags = MAP_SHARED;
    if (file.address != nullptr) {
        flags |= MAP_FIXED;
    }
    const auto ptr = mmap(file.address, file.max_size, PROT_READ | PROT_WRITE, flags, fd, 0);
    close(fd);
    if (file.address != nullptr and ptr != file.address) {
        throw std::runtime_error("error: could not pin the file at the desired pointer!");
    }
    m_resources.emplace_hint(m_resources.cend(),std::piecewise_construct,
                                    std::forward_as_tuple(m_aggregate_size),
                                    std::forward_as_tuple(ptr, file.max_size, m_aggregate_size));
    m_aggregate_size += file.max_size;
}

std::fstream mmap_storage::create_logger() const {
    auto flags = std::ios::in | std::ios::out;
    if (!std::filesystem::exists(m_log_file_path)) {
        flags |= std::ios::trunc;
    }
    return {m_log_file_path, flags};
}

void mmap_storage::replay_logger() {
    std::string token;
    while (m_log >> token) {
        if (token == "al") {
            size_t bytes;
            m_log >> bytes;
            do_allocate(bytes);
        } else if (token == "de") {
            size_t offset;
            size_t bytes;
            m_log >> offset >> bytes;
            do_deallocate(offset, bytes);
        } else {
            throw std::invalid_argument("error: unknown token in the allocation log");
        }
    }
    m_log.clear();
}

offset_ptr mmap_storage::do_allocate (size_t bytes) {

    for (auto &resource: m_resources) {
        try {
            auto ptr = resource.second.get_pool_resource().allocate (bytes);
            return resource.second.m_ptr.get_offset_ptr_at (ptr);
        }
        catch (std::bad_alloc &) {}
    }
    throw std::bad_alloc();
}

void mmap_storage::do_deallocate (const offset_ptr& offset_ptr, size_t bytes) {
    auto &resource = get_resource (offset_ptr.m_offset, bytes);
    const auto deallocate_offset_ptr = resource.m_ptr.get_offset_ptr_at(offset_ptr.m_offset);
    resource.get_pool_resource().deallocate(deallocate_offset_ptr.m_addr, bytes);
}

std::filesystem::path mmap_storage::generate_log_file_path (const std::forward_list<file_mmap_info> &files) {

    std::hash <std::filesystem::path> hasher;
    std::stringstream hash_stream;
    hash_stream << "log_";
    for (const auto &fi: files) {
        hash_stream << std::hex << hasher (fi.path);
    }
    return hash_stream.str();
}

bool mmap_storage::files_consistent_existency (const std::forward_list<file_mmap_info>& files) {
    bool existed_files = std::filesystem::exists(files.begin()->path);
    for (auto itr = std::next (files.cbegin()); itr != files.cend(); itr ++) {
        if (std::filesystem::exists(itr->path) != existed_files) {
            throw std::runtime_error ("error: the data files must be consistent in their existence!");
        }
    }
    return existed_files;
}

mmap_storage::resource_entry &mmap_storage::get_resource(size_t offset, size_t size) {
    auto itr = m_resources.upper_bound (offset);
    if (itr == m_resources.cbegin()) {
        throw std::domain_error("error: deallocate request for non-existing resource");
    }
    itr--;
    if (itr->first + itr->second.m_size < offset + size) {
        throw std::domain_error("error: deallocate request for non-existing resource");
    }
    return itr->second;
}

offset_ptr::offset_ptr(size_t offset, void *addr) :
        m_addr (static_cast <char*> (addr)), m_offset (offset) {}

offset_ptr offset_ptr::get_offset_ptr_at(size_t offset) const {
    if (m_addr == nullptr) {
        throw std::logic_error ("error: nullptr in offset_ptr");
    }
    return {offset, (offset - m_offset) + static_cast <char*> (m_addr)};
}

offset_ptr offset_ptr::get_offset_ptr_at(void *raw_ptr) const {
    if (m_addr == nullptr) {
        throw std::logic_error ("error: nullptr in offset_ptr");
    }
    return {(static_cast <char*> (raw_ptr) - static_cast <char*> (m_addr)) + m_offset, raw_ptr};
}

mmap_storage::resource_entry::resource_entry(void *addr, size_t size, size_t offset) :
        m_ptr (offset, addr),
        m_size (size),
        m_monotonic_buffer(addr, size, std::pmr::null_memory_resource()),
        m_pool_resource(&m_monotonic_buffer) {}

std::pmr::memory_resource &mmap_storage::resource_entry::get_pool_resource() {
    return m_pool_resource;
}


} // end namespace uh::dbn::storage::smart
