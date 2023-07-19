//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "cluster_config.h"
#include "common.h"
#include <span>
#include <memory_resource>
#include <map>
#include <fcntl.h>
#include <unistd.h>

namespace uh::cluster {


class data_store {

public:

    explicit data_store (int id, data_store_config conf):
        m_id (id),
        m_conf (std::move (conf)) {


        if (!std::filesystem::exists(m_conf.directory)) {
            std::filesystem::create_directories(m_conf.directory);
        }

        for (const auto& entry: std::filesystem::directory_iterator (m_conf.directory)) {
            if (entry.path() == m_conf.log_file) {
                continue;
            }

            const auto id_offset = parse_file_name (entry.path().filename());
            if (id_offset.first != m_id) {
                continue;
            }

            const int fd = open (entry.path().c_str(), O_RDWR | O_DSYNC);
            if (fd <= 0) {
                throw std::filesystem::filesystem_error ("Could not open the files in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }

            std::size_t data_size;
            const auto ret = ::read (fd, &data_size, sizeof (data_size));
            if (ret != sizeof (data_size)) {
                throw std::system_error (std::error_code(errno, std::system_category()), "Could not read the data size");
            }

            m_open_files.emplace(id_offset.second, fd);
            m_file_data_sizes.emplace(fd, data_size);
            m_file_sizes.emplace(fd,std::filesystem::file_size(entry.path()));
            m_file_paths.emplace(fd, std::filesystem::absolute(entry.path()));
        }

        if (m_open_files.empty()) {
            add_new_file(0, m_conf.min_file_size);
        }
    }

    address write (std::span <char> data) {
        const auto alloc = allocate (data.size());
        // TODO io_uring

        unsigned long offset = 0;
        for (const auto& partial_alloc: alloc) {
            lseek(partial_alloc.fd, partial_alloc.offset, SEEK_CUR);
            ::write(partial_alloc.fd, data.data() + offset, partial_alloc.size);
            offset += partial_alloc.size;
        }

        return {};
    }

    std::span <char> read (uint128_t pointer, size_t size) const;

    void remove (uint128_t pointer, size_t size);

    ~data_store() {
        for (const auto& open_file: m_open_files) {
            fsync (open_file.second);
            close (open_file.second);
        }
    }


private:

    struct partial_alloc_t {
        int fd;
        std::int64_t offset;
        std::size_t size;
    };
    typedef std::forward_list <partial_alloc_t> alloc_t;

    alloc_t allocate (std::size_t size) {

        // TODO later we will have a list of free spots. First go through them

        alloc_t alloc;
        // TODO check if we have space for size
        
        while (size > 0) {
            auto partial_size = std::min(size, m_conf.max_file_size);
            size -= partial_size;

            const auto last_fd = m_open_files.crbegin()->second;
            auto &data_size = m_file_data_sizes.at(last_fd);
            auto &file_size = m_file_sizes.at(last_fd);

            if (data_size + partial_size <= file_size) {                    // write in last file

                alloc.emplace_front(last_fd, data_size, partial_size);
            } else if (file_size < m_conf.max_file_size) {            // double the file size

                const auto new_file_size = file_size << 1;
                const int rc = ftruncate(last_fd, static_cast <long> (new_file_size));
                if (rc != 0) {
                    throw std::filesystem::filesystem_error("Could not extend file in the data store",
                                                            std::error_code(errno, std::system_category()));
                }
                file_size = new_file_size;
                alloc.emplace_front(last_fd, data_size, partial_size);

            } else if (const auto total_size = m_open_files.size() * m_conf.max_file_size; total_size + partial_size <
                                                                                           m_conf.max_storage_size) {            // create a new file

                auto init_file_size = m_conf.min_file_size;
                while (init_file_size < partial_size and init_file_size < m_conf.max_file_size) {
                    init_file_size <<= 1;
                }

                const auto fd = add_new_file(total_size, init_file_size);
                alloc.emplace_front(fd, 0, partial_size);

            } else {                                                  // no space left
                throw std::bad_alloc();
            }

            data_size += partial_size;
            const auto ret = ::write(last_fd, &data_size, sizeof(data_size));
            if (ret != sizeof(data_size)) {
                throw std::system_error(std::error_code(errno, std::system_category()),
                                        "Could not write the data size");
            }
        }

        return alloc;
    }

    int add_new_file (const uint128_t& offset, std::size_t file_size) {
        const auto file_path = m_conf.directory / get_name(offset);
        const int fd = open (file_path.c_str(), O_RDWR | O_CREAT | O_DSYNC, S_IWUSR | S_IRUSR);
        if (fd <= 0) {
            throw std::filesystem::filesystem_error ("Could not create new files in the data store root",
                                                     std::error_code(errno, std::system_category()));
        }

        const int rc = ftruncate(fd, file_size);
        if (rc != 0) {
            throw std::filesystem::filesystem_error ("Could not truncate new files in the data store root",
                                                     std::error_code(errno, std::system_category()));
        }

        std::size_t data_size = 0;
        const auto ret = ::write(fd, &data_size, sizeof(data_size));
        if (ret != sizeof(data_size)) {
            throw std::system_error (std::error_code(errno, std::system_category()), "Could not write the data size");
        }
        m_open_files.emplace(0, fd);
        m_file_data_sizes.emplace(fd, 0);
        m_file_sizes.emplace(fd, file_size);
        m_file_paths.emplace(fd, std::filesystem::absolute(file_path));
        return fd;
    }

    [[nodiscard]] static std::pair <int, uint128_t> parse_file_name (const std::string& filename) {
        const auto index1 = filename.find('_') + 1;
        const auto index2 = filename.substr(index1).find('_') + index1 + 1;
        const auto id_str = filename.substr(index1, index2 - 1 - index1);
        const auto offset_str = filename.substr(index2);
        return {std::stoi (id_str), big_int (offset_str)};
    }

    [[nodiscard]] std::string get_name (const uint128_t& offset) const {
        return "data_" + std::to_string(m_id) + "_" + offset.to_string();
    }

    const int m_id;
    const data_store_config m_conf;
    std::map <uint128_t, int> m_open_files;
    std::unordered_map <int, std::size_t> m_file_data_sizes;
    std::unordered_map <int, std::size_t> m_file_sizes;
    std::unordered_map <int, std::filesystem::path> m_file_paths;


};

} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
