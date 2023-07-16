//
// Created by masi on 6/8/23.
//

#include "storage_common.h"
#include <boost/interprocess/mapped_region.hpp>

namespace uh::dbn::storage::smart {

void *align_ptr(void *ptr) noexcept {
    static const size_t PAGE_SIZE = boost::interprocess::mapped_region::get_page_size();
    return static_cast <char*> (ptr) - reinterpret_cast <size_t> (ptr) % PAGE_SIZE;
}

void sync_ptr (void *ptr, std::size_t size) {
    /*
    const auto aligned = align_ptr (ptr);
    if (msync(aligned, static_cast <char*> (ptr) - static_cast <char*> (aligned) + size, MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "could not sync the mmap data");
    }
     */
}


offset_ptr::offset_ptr(uint64_t offset, void *addr) :
        m_offset (offset), m_addr (static_cast <char*> (addr)) {}

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

bool offset_ptr::operator==(const offset_ptr &ptr) const noexcept {
    return m_addr == ptr.m_addr;
}

resource_entry::resource_entry(void *addr, std::filesystem::path path, size_t size, size_t offset) :
    m_path (std::move(path)),
    m_ptr (offset, addr),
    m_size (size),
    m_monotonic_buffer(addr, size, std::pmr::null_memory_resource()),
    m_pool_resource(&m_monotonic_buffer) {}

std::pmr::memory_resource& resource_entry::get_pool_resource() {
    return m_pool_resource;
}

void managed_storage::mmap_file(const std::filesystem::path &file, uint64_t offset, size_t file_size) {

    const auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (ftruncate(fd, file_size) != 0) {
        std::filesystem::filesystem_error (errno);
    }
    const auto ptr = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    m_resources.emplace_hint(m_resources.cend(),std::piecewise_construct,
                             std::forward_as_tuple(offset),
                             std::forward_as_tuple(ptr, file, file_size, offset));
}

resource_entry &managed_storage::get_resource(size_t offset, size_t size) {
    auto itr = m_resources.upper_bound (offset);
    if (itr == m_resources.cbegin()) {
        throw std::domain_error("error: request for non-existing resource");
    }
    itr--;
    if (itr->first + itr->second.m_size < offset + size) {
        throw std::domain_error("error: request for non-existing resource");
    }
    return itr->second;
}

void managed_storage::sync() {
    /*
    for (auto &resource: m_resources) {
        if (msync (resource.second.m_ptr.m_addr, resource.second.m_size, MS_SYNC) != 0) {
            throw std::system_error (errno, std::system_category(), "managed_storage could not sync the mmap data");
        }
    }
     */
}

} // end namespace uh::dbn::storage::smart
