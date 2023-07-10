//
// Created by masi on 5/27/23.
//
#include "growing_plain_storage.h"
#include <cstring>

namespace uh::dbn::storage::smart {

growing_plain_storage::growing_plain_storage(growing_plain_storage_config conf) :
        m_file_path (std::move (conf.file)),
        m_file_size (conf.init_size),
        m_storage (init_mmap(m_file_path, conf.init_size, m_file_size)) {

}

growing_plain_storage::growing_plain_storage(growing_plain_storage &&storage) noexcept:
        m_file_path (std::move (storage.m_file_path)),
        m_file_size (storage.get_size()),
        m_storage (storage.m_storage) {
    storage.m_file_size = 0;
    storage.m_storage = nullptr;
}

growing_plain_storage &growing_plain_storage::operator=(growing_plain_storage &&storage) noexcept {
    m_file_size = storage.m_file_size;
    m_file_path = std::move (storage.m_file_path);
    m_storage = storage.m_storage;
    storage.m_file_size = 0;
    storage.m_storage = nullptr;
    return *this;
}

void growing_plain_storage::destroy() {
    munmap (m_storage, m_file_size);
    std::filesystem::remove(m_file_path);
    m_file_size = 0;
    m_storage = nullptr;
}

char *growing_plain_storage::init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size) {

    const auto existing_storage = std::filesystem::exists(file_path.c_str());
    if (!existing_storage and !file_path.parent_path().empty()) {
        std::filesystem::create_directories(file_path.parent_path());
    }
    const auto fd = open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (!existing_storage) {
        file_size = init_size;
        if (ftruncate(fd, file_size) != 0) {
            std::filesystem::filesystem_error (errno);
        }
    }
    else {
        file_size = lseek (fd, 0, SEEK_END);
    }

    const auto mmap_addr = static_cast <char*> (mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    if (!existing_storage) {
        std::memset (mmap_addr, 0, init_size);
    }
    return mmap_addr;
}

void growing_plain_storage::extend_mapping() {

    msync (m_storage, m_file_size, MS_SYNC);
    munmap (m_storage, m_file_size);

    m_file_size *= 2;

    const auto fd = open(m_file_path.c_str(), O_RDWR);
    if (ftruncate(fd, m_file_size) != 0) {
        std::filesystem::filesystem_error (errno);
    }
    m_storage = static_cast <char*> (mmap(nullptr, m_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

}

growing_plain_storage::~growing_plain_storage() {
    if (m_storage != nullptr) {
        msync(m_storage, m_file_size, MS_SYNC);
        munmap(m_storage, m_file_size);
    }
}

void growing_plain_storage::rename_file(const std::filesystem::path &new_name) {
    std::filesystem::rename(m_file_path, new_name);
    m_file_path = new_name;
}


} // end namespace uh::dbn::storage::smart
