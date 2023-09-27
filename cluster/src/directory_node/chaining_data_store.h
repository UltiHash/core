//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "common/cluster_config.h"
#include "common/common.h"
#include <data_node/free_spot_manager.h>
#include <span>
#include <list>
#include <memory_resource>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace uh::cluster {


struct chaining_data_store_config {
    std::filesystem::path directory;
    std::filesystem::path free_spot_log;
    size_t min_file_size;
    size_t max_file_size;
    size_t max_storage_size;
    size_t max_chunk_size;
};

class chaining_data_store {

    using index_type = uint64_t;
    using size_type = size_t;

    struct header {

        constexpr static size_t small_size = sizeof (uint32_t);
        constexpr static size_t large_size = sizeof (uint32_t) + sizeof (uint64_t);
        constexpr static size_t root_size = sizeof (uint32_t) + sizeof (uint64_t) + sizeof (size_t);


        uint32_t has_next: 1;
        uint32_t size: 31;
        uint64_t next_ptr;
        size_t total_size = 0;

        [[nodiscard]] inline std::vector <uint32_t> serialize () const {
            std::vector <uint32_t> buf {1};
            buf [0] |= has_next << 31;
            buf [0] |= size;
            if (has_next) [[unlikely]] {
                buf.resize (3);
                std::memcpy(buf.data() + 1, &next_ptr, sizeof(next_ptr));
                if (total_size) {
                    buf.resize (5);
                    std::memcpy(buf.data() + 3, &total_size, sizeof(total_size));
                }
            }
            return buf;
        }

        inline void load_from_file (const int fd, bool root_header) {
            uint32_t buf;
            const auto r1 = ::read (fd, &buf, sizeof (buf));
            if (r1 != sizeof (buf)) [[unlikely]] {
                throw std::runtime_error ("Could not read the header from file");
            }
            has_next = (buf & (1 << 31)) >> 31;
            size = buf & ~(1 << 31);
            if (has_next) [[unlikely]] {
                const auto r2 = ::read (fd, &next_ptr, sizeof (next_ptr));
                if (r2 != sizeof (next_ptr)) [[unlikely]] {
                    throw std::runtime_error ("Could not read the header from file");
                }
                if (root_header) {
                    const auto r3 = ::read (fd, &total_size, sizeof (total_size));
                    if (r3 != sizeof (total_size)) [[unlikely]] {
                        throw std::runtime_error ("Could not read the header from file");
                    }
                }
            }
        }

    };

public:

    explicit chaining_data_store (chaining_data_store_config conf):
            m_conf (std::move (conf)),
            m_free_spot_manager (m_conf.free_spot_log) {

        if (!std::filesystem::exists(m_conf.directory)) {
            std::filesystem::create_directories(m_conf.directory);
        }

        std::unordered_map <int, std::size_t> file_sizes;
        for (const auto& entry: std::filesystem::directory_iterator (m_conf.directory)) {
            if (!is_data_file(entry.path())) {
                continue;
            }

            const int fd = open (entry.path().c_str(), O_RDWR);
            if (fd <= 0) [[unlikely]] {
                throw std::filesystem::filesystem_error ("Could not open the files in the chaining data store root",
                                                         std::error_code(errno, std::system_category()));
            }

            const auto id_offset = parse_file_name (entry.path().filename());
            m_open_files.emplace(id_offset.second, fd);
            file_sizes.emplace(fd, std::filesystem::file_size(entry.path()));
        }

        if (m_open_files.empty()) {
            int fd = add_new_file(0, static_cast <long> (m_conf.min_file_size));
            file_sizes.emplace(fd, m_conf.min_file_size);
        }
        else {
            std::tie (m_last_file_offset, m_last_fd) = *m_open_files.crbegin();
            m_last_file_size = file_sizes.at(m_last_fd);
            const auto ret = ::read (m_last_fd, &m_last_file_data_end, sizeof (m_last_file_data_end));
            if (ret != sizeof (m_last_file_data_end)) {
                throw std::system_error (std::error_code(errno, std::system_category()), "Could not read the data size");
            }
        }

        m_used = fetch_used_space();


    }

    index_type write (std::span <char> data) {

        std::lock_guard <std::shared_mutex> lock (m);

        if (m_used + data.size() > m_conf.max_storage_size) [[unlikely]] {
            throw std::bad_alloc();
        }

        const auto alloc = allocate (data.size());

        unsigned long offset = 0;
        int index = 0;
        size_t total_acquired_size = 0;
        for (const auto& partial_alloc: alloc) {
            if (lseek(partial_alloc.fd, partial_alloc.seek, SEEK_SET) != partial_alloc.seek) [[unlikely]] {
                throw std::runtime_error ("Could not seek to the allocated offset");
            }
            header h;
            h.has_next = index < (alloc.size() - 1);
            size_t header_size = 0;
            if (h.has_next) [[unlikely]] {
                if (index == 0) {
                    header_size = header::root_size;
                }
                else {
                    header_size = header::large_size;
                }
            }
            else {
                header_size = header::small_size;
            }
            h.size = partial_alloc.size - header_size;
            if (h.has_next) {
                h.next_ptr = alloc [index + 1].global_offset;
                if (index == 0) {
                    h.total_size = data.size();
                }
            }
            const auto header_buffer = h.serialize();
            const auto ws = ::write (partial_alloc.fd, header_buffer.data(), header_size);
            if (ws != header_size) [[unlikely]] {
                throw std::runtime_error ("Could not write the header in the chaining data store");
            }
            for (size_t written = 0;
                 written < h.size;
                 written += ::write(partial_alloc.fd, data.data() + offset + written, h.size - written));
            offset += h.size;
            total_acquired_size += partial_alloc.size;
            index ++;
        }

        m_free_spot_manager.apply_popped_items();
        m_used += total_acquired_size;

        return alloc.begin()->global_offset;

    }

    ospan <char> read (index_type index) const {
        const auto [fd, seek] = get_file_offset_pair(index);
        if (::lseek (fd, seek, SEEK_SET) != seek) [[unlikely]] {
            throw std::runtime_error ("Could not seek to the read position.");
        }

        header h;
        h.load_from_file (fd, true);
        ospan <char> buf;
        if (h.has_next) [[unlikely]] {
            buf.resize(h.total_size);
        }
        else [[likely]] {
            buf.resize(h.size);
        }

        index_type offset = 0;
        size_t tr = 0;
        do {
            const auto r = ::read (fd, buf.data.get() + offset, h.size - tr);
            if (r == 0) [[unlikely]] {
                break;
            }
            tr += r;
            offset += r;
        } while (tr < h.size);

        while (h.has_next) {
            const auto [next_fd, next_seek] = get_file_offset_pair(h.next_ptr);
            if (::lseek (next_fd, next_seek, SEEK_SET) != next_seek) [[unlikely]] {
                throw std::runtime_error ("Could not seek to the read position.");
            }
            h.load_from_file(next_fd, false);

            tr = 0;
            do {
                const auto r = ::read (fd, buf.data.get() + offset, h.size - tr);
                if (r == 0) [[unlikely]] {
                    break;
                }
                tr += r;
                offset += r;
            } while (tr < h.size);
        }

        return buf;
    }

    void remove (index_type index);

    void sync ();

    [[nodiscard]] size_type get_used_space () const noexcept;


    ~chaining_data_store();


private:


    struct partial_alloc_t {
        int fd {};
        std::int64_t seek {};
        std::size_t size {};
        uint64_t global_offset;
    };
    typedef std::vector <partial_alloc_t> alloc_t;

    [[nodiscard]] std::pair <int, long> get_file_offset_pair (uint128_t pointer) const;

    [[nodiscard]] size_type fetch_used_space () const noexcept;

    alloc_t allocate (std::size_t size) {

        alloc_t alloc;

        auto required_size = size + header::small_size;
        size_t account_total_size = 1;

        auto free_spot = m_free_spot_manager.pop_free_spot();
        std::size_t partial_size = 0;
        while (free_spot.has_value() and required_size > 0) {
            const auto [fd, seek] = get_file_offset_pair(free_spot->pointer);
            partial_size = std::min (required_size, free_spot->size);
            alloc.emplace_back (fd, seek, partial_size, free_spot->pointer.get_data() [1]);

            required_size -= partial_size;
            if (required_size > 0) {
                required_size += header::large_size + account_total_size * sizeof (size);
                account_total_size = 0;
            }

            free_spot = m_free_spot_manager.pop_free_spot();
        }

        if (free_spot.has_value() and partial_size < free_spot->size) [[unlikely]] {
            m_free_spot_manager.push_free_spot (free_spot->pointer + partial_size,
                                               free_spot->size - partial_size);
        }

        auto last_file_data_end = m_last_file_data_end;
        while (required_size > 0) {
            partial_size = std::min ({m_conf.max_chunk_size, required_size, m_conf.max_file_size - last_file_data_end});

            if (partial_size == 0) [[unlikely]] {
                //TODO revert the new file if IO not successful
                m_modified_files.insert_or_assign (m_last_fd, last_file_data_end);

                add_new_file (m_conf.max_file_size * m_open_files.size(),
                              static_cast <long> (m_conf.min_file_size));
                m_used += sizeof (last_file_data_end);
                partial_size = std::min ({m_conf.max_chunk_size, required_size, m_conf.max_file_size});
                last_file_data_end = m_last_file_data_end;
            }

            required_size -= partial_size;
            if (required_size > 0) [[unlikely]] {
                required_size += header::large_size + account_total_size * sizeof (size);
                account_total_size = 0;
            }

            if (last_file_data_end + partial_size <= m_last_file_size) [[likely]] {   // write in last file
                alloc.emplace_back (m_last_fd, last_file_data_end, partial_size,
                                    m_last_file_offset + last_file_data_end);

            } else if (m_last_file_size < m_conf.max_file_size) {                       // double the file size
                auto new_file_size = m_last_file_size << 1;
                while (new_file_size - last_file_data_end < partial_size and new_file_size < m_conf.max_file_size) {
                    new_file_size <<= 1;
                }
                const int rc = ftruncate(m_last_fd, static_cast <long> (new_file_size));
                if (rc != 0) [[unlikely]] {
                    throw std::filesystem::filesystem_error("Could not extend file in the data store",
                                                            std::error_code(errno, std::system_category()));
                }
                m_last_file_size = new_file_size;
                alloc.emplace_back (m_last_fd, last_file_data_end, partial_size, m_last_file_offset + last_file_data_end);

            } else [[unlikely]] {
                throw std::logic_error("No place found to write the data");
            }
            last_file_data_end += partial_size;
        }

        m_last_file_data_end = last_file_data_end;
        m_modified_files.insert_or_assign (m_last_fd, last_file_data_end);

        return alloc;
    }

    int add_new_file (const index_type& offset, long file_size);

    [[nodiscard]] static std::pair <int, index_type> parse_file_name (const std::string& filename);

    [[nodiscard]] std::string get_name (const index_type& offset) const;

    static bool is_data_file (const std::filesystem::path& path);

    int m_last_fd {};
    std::size_t m_last_file_data_end {};
    long m_data_id;
    const chaining_data_store_config m_conf;
    free_spot_manager m_free_spot_manager;
    std::map <uint64_t, int> m_open_files;
    std::unordered_map <int, std::size_t> m_modified_files;
    uint64_t m_last_file_offset;
    std::size_t m_last_file_size;
    uint64_t m_used;
    std::shared_mutex m;

};

} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
