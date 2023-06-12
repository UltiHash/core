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
    const auto aligned = align_ptr (ptr);
    if (msync(aligned, static_cast <char*> (ptr) - static_cast <char*> (aligned) + size, MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "could not sync the mmap data");
    }
}

void managed_storage::mmap_file(const std::filesystem::path &file, uint64_t offset, size_t file_size) {

    const auto fd = open(file.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(fd, file_size);
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

} // end namespace uh::dbn::storage::smart
