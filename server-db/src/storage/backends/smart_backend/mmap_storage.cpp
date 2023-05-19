//
// Created by masi on 5/15/23.
//
#include <unordered_map>
#include "mmap_storage.h"

namespace uh::dbn::storage::smart {

mmap_storage::mmap_storage(const std::forward_list<file_mmap_info> &files):
    m_init_run (!files_consistent_existency(files)),
    m_log_file_path (generate_log_file_path(files)),
    m_log(create_logger()) {

    files_consistent_existency(files);
    for (const auto &file: files) {
        mmap_file (file);
    }

    replay_logger();
}

void* mmap_storage::allocate(std::size_t size) {
    m_log << "al " << size << '\n';
    return do_allocate(size);
}

void mmap_storage::deallocate(void *p, size_t size) {
    m_log << "de " << m_resource_container.get_init_ptr (p) << ' ' << size << '\n';
    do_deallocate(p, size);
}

void mmap_storage::sync(void *ptr, std::size_t size) {
    msync(ptr, size, MS_SYNC);
}

void mmap_storage::mmap_file(const file_mmap_info &file) {

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
    if (m_init_run) {
        m_log << "init " << file.path << ' ' << ptr << '\n';
    }
    m_resource_container.add_resource(file.path, ptr, file.max_size);
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
        if (token == "init") {
            std::filesystem::path p;
            void* addr;
            m_log >> p >> addr;
            m_resource_container.set_init_address(p, addr);
        }
        else if (token == "al") {
            size_t bytes;
            m_log >> bytes;
            do_allocate(bytes);
        } else if (token == "de") {
            void* addr;
            size_t bytes;
            m_log >> addr >> bytes;
            do_deallocate(m_resource_container.get_current_ptr(addr), bytes);
        } else {
            throw std::invalid_argument("error: unknown token in the allocation log");
        }
    }
    m_log.clear();
}

void* mmap_storage::do_allocate(size_t bytes) {

    for (auto &resource: m_resource_container) {
        try {
            return resource.allocate (bytes);
        }
        catch (std::bad_alloc &) {}
    }
    throw std::bad_alloc();
}

void mmap_storage::do_deallocate(void *p, size_t bytes) {
    auto &resource = m_resource_container.get_resource (p, bytes);
    resource.deallocate(p, bytes);
}

std::filesystem::path mmap_storage::generate_log_file_path(const std::forward_list<file_mmap_info> &files) {

    std::hash <std::filesystem::path> hasher;
    std::stringstream hash_stream;
    hash_stream << "log_";
    for (const auto &fi: files) {
        hash_stream << std::hex << hasher (fi.path);
    }
    return hash_stream.str();
}

bool mmap_storage::files_consistent_existency(const std::forward_list<file_mmap_info> &files) {
    bool existed_files = std::filesystem::exists(files.begin()->path);
    for (auto itr = std::next (files.cbegin()); itr != files.cend(); itr ++) {
        if (std::filesystem::exists(itr->path) != existed_files) {
            throw std::runtime_error ("error: the data files must be consistent in their existence!");
        }
    }
    return existed_files;
}

} // end namespace uh::dbn::storage::smart
