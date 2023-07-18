//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "cluster_config.h"
#include <span>
#include <memory_resource>
#include <map>
#include <fcntl.h>
#include <unistd.h>

namespace uh::cluster {

struct big_span {
    uint128_t pointer;
    size_t size;
};

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

            const int fd = open (entry.path().c_str(), O_RDWR | O_DIRECT | O_DSYNC);
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
            m_file_sizes.emplace(fd, data_size);
        }

        if (m_open_files.empty()) {
            const auto file_path = m_conf.directory / get_name(0);
            const int fd = open (file_path.c_str(), O_RDWR | O_CREAT | O_DIRECT | O_DSYNC, S_IWUSR | S_IRUSR);
            if (fd <= 0) {
                throw std::filesystem::filesystem_error ("Could not create new files in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }

            const int rc = ftruncate(fd, m_conf.min_file_size);
            if (rc != 0) {
                throw std::filesystem::filesystem_error ("Could not truncate new files in the data store root",
                                                         std::error_code(errno, std::system_category()));
            }

            alignas (FS_ALIGNMENT) char buf [FS_ALIGNMENT];
            std::memset (buf, 0, sizeof (std::size_t));
            const auto ret = ::write(fd, buf, FS_ALIGNMENT);
            if (ret != FS_ALIGNMENT) {
                throw std::system_error (std::error_code(errno, std::system_category()), "Could not write the data size");
            }
            m_open_files.emplace(0, fd);
            m_file_sizes.emplace(fd, 0);
        }
    }

    uint128_t write (std::span <char> data);
    big_span read (uint128_t pointer, size_t size) const;

    ~data_store() {
        for (const auto& open_file: m_open_files) {
            close (open_file.second);
        }
    }


private:

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
    constexpr static std::size_t FS_ALIGNMENT = 4096;
    std::map <uint128_t, int> m_open_files;
    std::unordered_map <int, std::size_t> m_file_sizes;

};

} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
