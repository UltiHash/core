//
// Created by masi on 5/27/23.
//

#ifndef CORE_GROWING_PLAIN_STORAGE_H
#define CORE_GROWING_PLAIN_STORAGE_H

#include <filesystem>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace uh::dbn::storage::smart {

class growing_plain_storage {


    const std::filesystem::path m_file_path;
    size_t m_file_size;
    char* m_storage;

public:

    growing_plain_storage (std::filesystem::path file, size_t init_size);


    [[nodiscard]] inline char* get_storage () const noexcept {
        return m_storage;
    }

    [[nodiscard]] inline size_t get_size () const noexcept {
        return m_file_size;
    }

    static char* init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size);

    void extend_mapping();

    ~growing_plain_storage();

};
} // end namespace uh::dbn::storage::smart

#endif //CORE_GROWING_PLAIN_STORAGE_H
