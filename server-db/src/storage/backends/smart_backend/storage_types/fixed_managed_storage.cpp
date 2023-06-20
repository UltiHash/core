//
// Created by masi on 5/15/23.
//
#include <unordered_map>
#include "fixed_managed_storage.h"

namespace uh::dbn::storage::smart {



fixed_managed_storage::fixed_managed_storage(fixed_managed_storage_config conf):
    m_conf(std::move (conf)),
    m_log(create_logger()) {
    std::filesystem::create_directories(m_conf.data_store_files.front().parent_path());
    files_existence_consistency ();
    for (const auto &file: m_conf.data_store_files) {
        mmap_file (file, m_aggregated_size, m_conf.data_store_file_size);
        m_aggregated_size += m_conf.data_store_file_size;
    }

    replay_logger();
}

offset_ptr fixed_managed_storage::allocate(std::size_t size) {
    m_log << "al " << size << '\n';
    return do_allocate(size);
}

void fixed_managed_storage::deallocate(const offset_ptr& off_ptr, size_t size) {
    m_log << "de " << off_ptr.m_offset << ' ' << size << '\n';
    do_deallocate(off_ptr, size);
}

void fixed_managed_storage::sync(void *ptr, std::size_t size) {
    sync_ptr(ptr, size);
}

void fixed_managed_storage::sync () {
    managed_storage::sync();
}

void *fixed_managed_storage::get_raw_ptr(size_t offset) {
    return get_resource(offset).m_ptr.get_offset_ptr_at(offset).m_addr;
}

std::fstream fixed_managed_storage::create_logger() const {
    auto flags = std::ios::in | std::ios::out;
    if (!std::filesystem::exists(m_conf.log_file)) {
        flags |= std::ios::trunc;
    }
    return {m_conf.log_file, flags};
}

void fixed_managed_storage::replay_logger() {
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

offset_ptr fixed_managed_storage::do_allocate (size_t bytes) {

    for (auto &resource: m_resources) {
        try {
            auto ptr = resource.second.get_pool_resource().allocate (bytes);
            return resource.second.m_ptr.get_offset_ptr_at (ptr);
        }
        catch (std::bad_alloc &) {}
    }
    throw std::bad_alloc();
}

void fixed_managed_storage::do_deallocate (const offset_ptr& offset_ptr, size_t bytes) {
    auto &resource = get_resource (offset_ptr.m_offset, bytes);
    const auto deallocate_offset_ptr = resource.m_ptr.get_offset_ptr_at(offset_ptr.m_offset);
    resource.get_pool_resource().deallocate(deallocate_offset_ptr.m_addr, bytes);
}

bool fixed_managed_storage::files_existence_consistency () {
    bool existed_files = std::filesystem::exists(*m_conf.data_store_files.cbegin());
    for (auto itr = m_conf.data_store_files.cbegin(); itr != m_conf.data_store_files.cend(); itr ++) {
        if (std::filesystem::exists(*itr) != existed_files) {
            throw std::runtime_error ("error: the data files must be consistent in their existence!");
        }
    }
    return existed_files;
}

fixed_managed_storage::~fixed_managed_storage() {
    sync();
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

resource_entry::resource_entry(void *addr, std::filesystem::path path, size_t size, size_t offset) :
        m_path (std::move(path)),
        m_ptr (offset, addr),
        m_size (size),
        m_monotonic_buffer(addr, size, std::pmr::null_memory_resource()),
        m_pool_resource(&m_monotonic_buffer) {}

std::pmr::memory_resource& resource_entry::get_pool_resource() {
    return m_pool_resource;
}

} // end namespace uh::dbn::storage::smart
