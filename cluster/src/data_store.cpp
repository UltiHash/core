//
// Created by masi on 7/21/23.
//

#include "data_store.h"

namespace uh::cluster {

data_store::data_store(data_store_config conf, int id, bool adaptive) :
        m_data_id (id),
        m_conf (std::move (conf)),
        m_free_spot_manager (m_conf.hole_log) {

    if (!std::filesystem::exists(m_conf.directory)) {
        std::filesystem::create_directories(m_conf.directory);
    }

    std::unordered_map <int, std::size_t> file_sizes;
    for (const auto& entry: std::filesystem::directory_iterator (m_conf.directory)) {
        if (!is_data_file(entry.path())) {
            continue;
        }

        const auto id_offset = parse_file_name (entry.path().filename());
        if (!adaptive and id_offset.first != m_data_id) [[unlikely]] { // non-adaptive data store with fixed id
            throw std::range_error ("The data store is spawned on the wrong data node");
        }
        else if (adaptive) { // data store with adaptive id (assuming one data store per node)
            adaptive = false;
            m_data_id = id_offset.first;
        }

        const int fd = open (entry.path().c_str(), O_RDWR);
        if (fd <= 0) [[unlikely]] {
            throw std::filesystem::filesystem_error ("Could not open the files in the data store root",
                                                     std::error_code(errno, std::system_category()));
        }

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

address data_store::write(std::span<char> data) {

    std::lock_guard <std::shared_mutex> lock (m);

    if (m_used + data.size() > m_conf.max_data_store_size) [[unlikely]] {
        throw std::bad_alloc();
    }

    const auto alloc = allocate (data.size());
    // TODO io_uring

    unsigned long offset = 0, index = 0;
    address data_address (alloc.size());
    for (const auto& partial_alloc: alloc) {
        if (lseek(partial_alloc.fd, partial_alloc.offset, SEEK_SET) != partial_alloc.offset) [[unlikely]] {
            throw std::runtime_error ("Could not seek to the allocated offset");
        }
        for (size_t written = 0;
             written < partial_alloc.size;
             written += ::write(partial_alloc.fd, data.data() + offset + written, partial_alloc.size - written));
        offset += partial_alloc.size;
        data_address [index ++] = wide_span (partial_alloc.global_offset, partial_alloc.size);
    }

    m_free_spot_manager.apply_popped_items();
    m_used += data.size();

    return data_address;
}

std::size_t data_store::read(char *buffer, uint128_t pointer, size_t size) const {
    const auto [fd, seek] = get_file_offset_pair(pointer);
    if (::lseek (fd, seek, SEEK_SET) != seek) [[unlikely]] {
        throw std::runtime_error ("Could not seek to the read position.");
    }

    size_t tr = 0;
    do {
        const auto r = ::read (fd, buffer + tr, size - tr);
        if (r == 0) [[unlikely]] {
            break;
        }
        tr += r;
    } while (tr < size);

    return tr;
}

void data_store::remove(uint128_t pointer, size_t size) {

    const auto [fd, seek] = get_file_offset_pair(pointer);

    if ((fd == m_last_fd and m_last_file_data_end < seek + size)
        or (fd != m_last_fd and m_conf.max_file_size < seek + size)) [[unlikely]] {
        throw std::out_of_range ("The removal offset + size goes out of the file scope.");
    }
    if (lseek(fd, seek, SEEK_SET) != seek) [[unlikely]] {
        throw std::runtime_error ("Could not seek to the removal position.");
    }
    char buf [1024];
    std::memset (buf, 0, sizeof(buf));
    for (size_t written = 0; written < size; written += ::write(fd, buf, std::min (size - written, sizeof(buf))));

    m_free_spot_manager.push_free_spot(pointer, size);
    m_used = m_used - size;
    fsync (fd);
}

void data_store::sync() {

    for (const auto modification: m_modified_files) {

        const auto fd = modification.first;
        const auto data_end = modification.second;
        if (::lseek(fd, 0, SEEK_SET) != 0) [[unlikely]] {
            throw std::runtime_error ("Could not seek to the first position.");
        }
        const auto ret = ::write(fd, &data_end, sizeof(data_end));
        if (ret != sizeof(data_end)) [[unlikely]] {
            throw std::system_error (std::error_code(errno, std::system_category()), "Could not write the data size");
        }


        if (::lseek(fd, 8, SEEK_SET) != 8) [[unlikely]] {
            throw std::runtime_error ("Could not seek to the first position.");
        }

        fsync (fd);
    }
    m_modified_files.clear();
    m_free_spot_manager.sync();
}

uint128_t data_store::fetch_used_space() const noexcept {
    const auto prev_files_data_size = big_int (m_conf.max_file_size) * (m_open_files.size() - 1);
    return prev_files_data_size + m_last_file_data_end - m_free_spot_manager.total_free_spots();
}

data_store::~data_store() {
    sync ();
    for (const auto& open_file: m_open_files) {
        fsync (open_file.second);
        close (open_file.second);
    }
}

std::pair<int, long> data_store::get_file_offset_pair(uint128_t pointer) const {
    const auto pfd = m_open_files.upper_bound (pointer);
    if (pfd == m_open_files.cbegin()) [[unlikely]] {
        throw std::out_of_range ("The given data offset could not be found in this data store");
    }
    const auto [file_offset, fd] = *std::prev (pfd);
    const auto seek = static_cast <long> ((pointer - file_offset).get_data()[1]);

    return {fd, seek};
}

data_store::alloc_t data_store::allocate (std::size_t size) {

    alloc_t alloc;

    auto free_spot = m_free_spot_manager.pop_free_spot();
    std::size_t partial_size = 0;
    while (free_spot.has_value() and size > 0) {
        const auto [fd, seek] = get_file_offset_pair(free_spot->pointer);
        partial_size = std::min (size, free_spot->size);
        alloc.emplace_front(fd, seek, partial_size, free_spot->pointer);
        size -= partial_size;
        free_spot = m_free_spot_manager.pop_free_spot();
    }

    if (free_spot.has_value() and partial_size < free_spot->size) [[unlikely]] {
        m_free_spot_manager.push_free_spot(free_spot->pointer + partial_size,
                                           free_spot->size - partial_size);
    }

    auto last_file_data_end = m_last_file_data_end;
    while (size > 0) {

        partial_size = std::min (size, m_conf.max_file_size - last_file_data_end);

        if (partial_size == 0) [[unlikely]] {
            //TODO revert the new file if IO not successful
            m_modified_files.insert_or_assign (m_last_fd, last_file_data_end);

            add_new_file (big_int (m_conf.max_file_size) * m_open_files.size(),
                         static_cast <long> (m_conf.min_file_size));
            m_used += sizeof (last_file_data_end);
            partial_size = std::min (size, m_conf.max_file_size);
            last_file_data_end = m_last_file_data_end;
        }

        size -= partial_size;

        if (last_file_data_end + partial_size <= m_last_file_size) [[likely]] {   // write in last file
            alloc.emplace_front(m_last_fd, last_file_data_end, partial_size, m_last_file_offset + last_file_data_end);
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
            alloc.emplace_front(m_last_fd, last_file_data_end, partial_size, m_last_file_offset + last_file_data_end);

        } else [[unlikely]] {
            throw std::logic_error("No place found to write the data");
        }
        last_file_data_end += partial_size;
    }

    m_last_file_data_end = last_file_data_end;
    m_modified_files.insert_or_assign (m_last_fd, last_file_data_end);

    return alloc;
}

int data_store::add_new_file(const uint128_t &offset, long file_size) {
    const auto file_path = m_conf.directory / get_name(offset);
    const int fd = open (file_path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd <= 0) [[unlikely]] {
        throw std::filesystem::filesystem_error ("Could not create new files in the data store root",
                                                 std::error_code(errno, std::system_category()));
    }

    const int rc = ftruncate(fd, file_size);
    if (rc != 0) [[unlikely]] {
        throw std::filesystem::filesystem_error ("Could not truncate new files in the data store root",
                                                 std::error_code(errno, std::system_category()));
    }

    std::size_t data_end = sizeof (data_end);
    const auto ret = ::write(fd, &data_end, sizeof(data_end));
    if (ret != sizeof(data_end)) [[unlikely]] {
        throw std::system_error (std::error_code(errno, std::system_category()), "Could not write the data size");
    }
    m_open_files.emplace(offset, fd);
    m_last_file_data_end = data_end;
    m_last_file_size = file_size;
    m_last_fd = fd;
    m_last_file_offset = offset;

    return fd;
}

std::pair<int, uint128_t> data_store::parse_file_name(const std::string &filename) {
    const auto index1 = filename.find('_') + 1;
    const auto index2 = filename.substr(index1).find('_') + index1 + 1;
    const auto id_str = filename.substr(index1, index2 - 1 - index1);
    const auto offset_str = filename.substr(index2);
    return {std::stoi (id_str), big_int (offset_str)};
}

std::string data_store::get_name(const uint128_t &offset) const {
    return "data_" + std::to_string(m_data_id) + "_" + offset.to_string();
}

bool data_store::is_data_file(const std::filesystem::path &path) {
    return path.string().starts_with("data_");
}

uint128_t data_store::get_used_space() const noexcept {
    return m_used;
}

int data_store::get_data_id() const noexcept {
    return m_data_id;
}



} // end namespace uh::cluster