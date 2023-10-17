//
// Created by masi on 5/27/23.
//

#ifndef CORE_CLUSTER_GROWING_PLAIN_STORAGE_H
#define CORE_CLUSTER_GROWING_PLAIN_STORAGE_H

#include <filesystem>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "common/cluster_config.h"

namespace uh::cluster::dedupe {

class growing_plain_storage {

    std::filesystem::path m_file_path;
    size_t m_file_size;
    char* m_storage;

public:

    growing_plain_storage (growing_plain_storage_config);

    growing_plain_storage (growing_plain_storage&&) noexcept;

    growing_plain_storage& operator = (growing_plain_storage&&)  noexcept;

    [[nodiscard]] inline char* get_storage () const noexcept {
        return m_storage;
    }

    [[nodiscard]] inline size_t get_size () const noexcept {
        return m_file_size;
    }

    [[nodiscard]] inline std::filesystem::path get_filename () const {
        return m_file_path;
    }

    static char* init_mmap(const std::filesystem::path &file_path, size_t init_size, size_t &file_size);

    void extend_mapping();

    void destroy ();

    void rename_file (const std::filesystem::path& new_name);

    ~growing_plain_storage();

};
} // end namespace uh::cluster::dedupe

#endif //CORE_CLUSTER_GROWING_PLAIN_STORAGE_H
