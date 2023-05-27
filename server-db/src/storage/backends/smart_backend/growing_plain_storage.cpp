//
// Created by masi on 5/27/23.
//
#include "growing_plain_storage.h"

namespace uh::dbn::storage::smart {

growing_plain_storage::growing_plain_storage(std::filesystem::path file, size_t init_size) :
        m_file_path (std::move (file)),
        m_file_size (init_size),
        m_storage (init_mmap(m_file_path, init_size, m_file_size)) {}

char *growing_plain_storage::init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size) {
    const auto existing_storage = std::filesystem::exists(file_path.c_str());
    const auto fd = open(file_path.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (!existing_storage) {
        file_size = init_size;
        ftruncate(fd, file_size);
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
    ftruncate(fd, m_file_size);

    m_storage = static_cast <char*> (mmap(nullptr, m_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

}

growing_plain_storage::~growing_plain_storage() {
    msync (m_storage, m_file_size, MS_SYNC);
    munmap (m_storage, m_file_size);
}

} // end namespace uh::dbn::storage::smart
