//
// Created by masi on 5/25/23.
//
#include <unordered_map>
#include <fstream>
#include "growing_managed_storage.h"

namespace uh::dbn::storage::smart {

growing_managed_storage::growing_managed_storage (growing_managed_storage_config conf):
        m_min_file_size (conf.min_file_size),
        m_max_file_size (conf.max_file_size),
        m_directory (std::move (conf.directory)),
        m_log_file_path (std::move (conf.log_file)),
        m_max_storage_size (conf.max_storage_size) {
     load_data_store ();
     m_log = create_logger();
     replay_logger();
}

offset_ptr growing_managed_storage::allocate(std::size_t size) {
    m_log << "al " << size << '\n';
    return do_allocate(size);
}

void growing_managed_storage::deallocate(const offset_ptr& off_ptr, size_t size) {
    m_log << "de " << off_ptr.m_offset << ' ' << size << '\n';
    do_deallocate(off_ptr, size);
}

void growing_managed_storage::sync(void *ptr, std::size_t size) {
    sync_ptr (ptr, size);
}

void growing_managed_storage::sync () {
    managed_storage::sync();
}

void* growing_managed_storage::get_raw_ptr(size_t offset) {
    return get_resource(offset).m_ptr.get_offset_ptr_at(offset).m_addr;
}

std::fstream growing_managed_storage::create_logger() const {
    auto flags = std::ios::in | std::ios::out;
    if (!std::filesystem::exists(m_log_file_path)) {
        flags |= std::ios::trunc;
    }
    return {m_log_file_path, flags};
}

void growing_managed_storage::replay_logger() {
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

offset_ptr growing_managed_storage::do_allocate (size_t bytes) {

    std::lock_guard <std::mutex> lock (m_mutex);

    for (auto& resource: m_resources) {
        try {
            auto ptr = resource.second.get_pool_resource().allocate(bytes);
            return resource.second.m_ptr.get_offset_ptr_at(ptr);
        }
        catch (std::bad_alloc& e) {}
    }

    for (int i = 0; i < MAX_GROW_ATTEMPTS; ++i) {
        grow();
        try {
            const auto resource = m_resources.rbegin();
            auto ptr = resource->second.get_pool_resource().allocate(bytes);
            return resource->second.m_ptr.get_offset_ptr_at(ptr);
        }
        catch (std::bad_alloc& e) {}
    }
    throw std::bad_alloc {};

}

void growing_managed_storage::do_deallocate (const offset_ptr& offset_ptr, size_t bytes) {
    auto &resource = get_resource (offset_ptr.m_offset, bytes);
    const auto deallocate_offset_ptr = resource.m_ptr.get_offset_ptr_at(offset_ptr.m_offset);
    resource.get_pool_resource().deallocate(deallocate_offset_ptr.m_addr, bytes);
}

void growing_managed_storage::load_data_store () {
    std::filesystem::create_directories(std::filesystem::absolute (m_directory));
    if (std::filesystem::is_empty(m_directory)) {
        if (m_min_file_size > m_max_storage_size) {
            throw std::bad_alloc ();
        }
        mmap_file(get_file_name (0, m_min_file_size), 0, m_min_file_size);
        m_storage_size = m_min_file_size;
    }
    else {
        m_storage_size = 0;
        for (const auto& file: std::filesystem::directory_iterator (m_directory)) {
            if (file == m_log_file_path) {
                continue;
            }
            const auto offset_size = parse_file_name(file);
            mmap_file(file, offset_size.first, offset_size.second);
            m_storage_size += offset_size.second;
        }
    }
}
void growing_managed_storage::grow () {
    sync();

    const auto last_resource = m_resources.crbegin();
    const auto file_name = last_resource->second.m_path;
    const auto last_offset_size = parse_file_name(file_name);
    if (last_offset_size.second >= m_max_file_size) { // then, create a new file
        const auto tmp_new_size = m_storage_size + m_min_file_size;
        if (tmp_new_size > m_max_storage_size) {
            throw std::bad_alloc ();
        }
        const auto new_offset = last_offset_size.first + last_offset_size.second;
        mmap_file(get_file_name (new_offset, m_min_file_size), new_offset, m_min_file_size);
        m_storage_size = tmp_new_size;
        return;
    }
    else { // else, grow the last existing file
        const auto offset = last_offset_size.first;
        const auto new_size = last_offset_size.second * 2ul;
        const auto tmp_new_size = m_storage_size + new_size - last_offset_size.second;
        if (tmp_new_size > m_max_storage_size) {
            throw std::bad_alloc ();
        }
        const auto new_file_name = get_file_name (offset, new_size);
        std::filesystem::rename(file_name, new_file_name);
        m_resources.erase (std::next (last_resource).base());
        mmap_file(new_file_name, offset, new_size);
        m_storage_size = tmp_new_size;
    }
    replay_logger();
}

std::filesystem::path growing_managed_storage::get_file_name (uint64_t offset, size_t file_size) {
    return m_directory / std::string {"file_" + std::to_string (offset) + "_" + std::to_string(file_size)};
}

std::pair <uint64_t, size_t> growing_managed_storage::parse_file_name (const std::filesystem::path& path) {
    const auto file = path.filename();
    const auto index1 = file.string().find('_') + 1;
    const auto index2 = file.string().substr(index1).find('_') + index1 + 1;
    const auto offset_str = file.string().substr(index1, index2 - 1 - index1);
    const auto size_str = file.string().substr(index2);
    return {std::stoul (offset_str), std::stoul (size_str)};
}

growing_managed_storage::~growing_managed_storage() {
    sync();
}


} // end namespace uh::dbn::storage::smart
