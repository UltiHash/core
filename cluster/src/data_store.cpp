//
// Created by masi on 7/21/23.
//

#include "data_store.h"

namespace uh::cluster {

data_store::data_store(int id, data_store_config conf) :
        m_id (id),
        m_conf (std::move (conf)),
        m_free_spot_manager (m_conf.hole_log){

    if (!std::filesystem::exists(m_conf.directory)) {
        std::filesystem::create_directories(m_conf.directory);
    }

    for (const auto& entry: std::filesystem::directory_iterator (m_conf.directory)) {
        if (entry.path() == m_conf.hole_log) {
            continue;
        }

        const auto id_offset = parse_file_name (entry.path().filename());
        if (id_offset.first != m_id) {
            continue;
        }

        const int fd = open (entry.path().c_str(), O_RDWR);
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
    }

    if (m_open_files.empty()) {
        add_new_file(0, static_cast <long> (m_conf.min_file_size));
    }
}

address data_store::write(std::span<char> data) {

    std::lock_guard <std::shared_mutex> lock (m);

    const auto alloc = allocate (data.size());
    // TODO io_uring

    unsigned long offset = 0;
    address data_address;
    auto pos = data_address.cbegin();
    for (const auto& partial_alloc: alloc) {
        if (lseek(partial_alloc.fd, partial_alloc.offset, SEEK_CUR) != partial_alloc.offset) [[unlikely]] {
            throw std::runtime_error ("Could not seek to the allocated offset");
        }
        for (size_t written = 0;
             written < partial_alloc.size;
             written += ::write(partial_alloc.fd, data.data() + offset + written, partial_alloc.size - written));
        offset += partial_alloc.size;
        pos = data_address.emplace_after(pos, partial_alloc.global_offset, partial_alloc.size);
        m_modified_files.emplace_front(partial_alloc.fd);
    }

    m_free_spot_manager.apply_popped_items();

    return data_address;
}

ospan<char> data_store::read(uint128_t pointer, size_t size) const {
    const auto [fd, seek] = get_file_offset_pair(pointer);
    ospan <char> data (size);
    if (::lseek (fd, seek, SEEK_SET) != seek) {
        throw std::runtime_error ("Could not seek to the read position.");
    }
    for (size_t r = 0;
         r < size;
         r += ::read (fd, data.data.get() + r, size - r));
    return data;
}

void data_store::remove(uint128_t pointer, size_t size) {

    const auto [fd, seek] = get_file_offset_pair(pointer);

    if (seek + size > m_file_sizes.at(fd)) [[unlikely]] {
        throw std::out_of_range ("The removal offset + size goes out of the file scope.");
    }
    if (lseek(fd, seek, SEEK_SET) != seek) [[unlikely]] {
        throw std::runtime_error ("Could not seek to the removal position.");
    }
    char buf [1024];
    std::memset (buf, 0, sizeof(buf));
    for (size_t written = 0; written < size; written += ::write(fd, buf, std::min (size - written, sizeof(buf))));

    m_free_spot_manager.push_free_spot(pointer, size);
    m_modified_files.emplace_front(fd);

}

void data_store::sync() {
    for (const auto fd: m_modified_files) {
        fsync(fd);
    }
    m_free_spot_manager.sync();
}

uint128_t data_store::used_space() {
    const auto prev_files_data_size =  big_int (m_conf.max_file_size) * (m_open_files.size() - 1);
    const auto last_fd = m_open_files.crbegin()->second;
    const auto last_file_data_size =  m_file_data_sizes.at(last_fd);
    return prev_files_data_size + last_file_data_size + m_free_spot_manager.total_free_spots();
}

data_store::~data_store() {
    for (const auto& open_file: m_open_files) {
        fsync (open_file.second);
        close (open_file.second);
    }
}

std::pair<int, long> data_store::get_file_offset_pair(uint128_t pointer) const {
    const auto pfd = m_open_files.upper_bound (pointer);
    if (pfd == m_open_files.cbegin()) {
        throw std::out_of_range ("The given data offset could not be found in this data store");
    }
    const auto [file_offset, fd] = *std::prev (pfd);
    const auto seek = static_cast <long> ((file_offset - pointer).get_data()[1]);

    return {fd, seek};
}

data_store::alloc_t data_store::allocate(std::size_t size) {

    alloc_t alloc;

    auto free_spot = m_free_spot_manager.pop_free_spot();
    std::size_t partial_size;
    while (free_spot.has_value() and size > 0) {
        const auto [fd, seek] = get_file_offset_pair(free_spot->pointer);
        partial_size = std::min (size, free_spot->size);
        alloc.emplace_front(fd, seek, partial_size, free_spot->pointer);
        size -= partial_size;
    }

    if (partial_size < free_spot->size) {
        m_free_spot_manager.push_free_spot(free_spot->pointer + partial_size,
                                           free_spot->size - partial_size);
    }

    while (size > 0) {
        partial_size = std::min(size, m_conf.max_file_size);
        size -= partial_size;

        const auto last_fd = m_open_files.crbegin()->second;
        const auto last_file_offset = m_open_files.crbegin()->first;
        auto &data_size = m_file_data_sizes.at(last_fd);
        auto &file_size = m_file_sizes.at(last_fd);

        if (data_size + partial_size <= file_size) {                    // write in last file

            alloc.emplace_front(last_fd, data_size, partial_size, last_file_offset + data_size);
        } else if (file_size < m_conf.max_file_size) {                  // double the file size

            const auto new_file_size = file_size << 1;
            const int rc = ftruncate(last_fd, static_cast <long> (new_file_size));
            if (rc != 0) {
                throw std::filesystem::filesystem_error("Could not extend file in the data store",
                                                        std::error_code(errno, std::system_category()));
            }
            file_size = new_file_size;
            alloc.emplace_front(last_fd, data_size, partial_size, last_file_offset + data_size);

        } else if (const auto total_size = big_int (m_conf.max_file_size) * m_open_files.size();
                total_size + partial_size < m_conf.max_storage_size) {   // create a new file

            if (data_size < file_size) {    // add the remaining unused file to the free spots
                m_free_spot_manager.push_free_spot(last_file_offset + data_size, file_size - data_size);
            }
            auto init_file_size = m_conf.min_file_size;
            while (init_file_size < partial_size and init_file_size < m_conf.max_file_size) {
                init_file_size <<= 1;
            }

            const auto fd = add_new_file(total_size, static_cast <long> (init_file_size));
            alloc.emplace_front (fd, 0, partial_size, total_size);

        } else {                                                        // no space left
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

int data_store::add_new_file(const uint128_t &offset, long file_size) {
    const auto file_path = m_conf.directory / get_name(offset);
    const int fd = open (file_path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
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
    return "data_" + std::to_string(m_id) + "_" + offset.to_string();
}

} // end namespace uh::cluster