#include "data_store.h"
#include "common/telemetry/metrics.h"
#include <mutex>

namespace uh::cluster {

data_store::data_store(data_store_config conf, std::size_t id, bool adaptive)
    : m_data_id(id),
      m_conf(std::move(conf)) {

    m_open_files.reserve(2 * m_conf.max_data_store_size / m_conf.file_size + 1);
    m_global_offset = uint128_t(m_conf.max_data_store_size) * id;

    if (!std::filesystem::exists(m_conf.working_dir)) {
        std::filesystem::create_directories(m_conf.working_dir);
    }

    std::map<uint128_t, int> open_files;
    for (const auto& entry :
         std::filesystem::directory_iterator(m_conf.working_dir)) {
        if (!is_data_file(entry.path())) {
            continue;
        }

        const auto id_offset = parse_file_name(entry.path().filename());
        if (!adaptive and id_offset.first != m_data_id)
            [[unlikely]] { // non-adaptive data store with fixed id
            throw std::range_error(
                "The data store is spawned on the wrong data node");
        } else if (adaptive) { // data store with adaptive id (assuming one data
                               // store per node)
            adaptive = false;
            m_data_id = id_offset.first;
        }

        const int fd = open(entry.path().c_str(), O_RDWR);
        if (fd <= 0) [[unlikely]] {
            throw std::filesystem::filesystem_error(
                "Could not open the files in the data store root",
                std::error_code(errno, std::system_category()));
        }

        open_files.emplace(id_offset.second, fd);
    }

    for (const auto& of : open_files) {
        m_open_files.emplace_back(of.second);
    }

    if (m_open_files.empty()) {
        add_new_file(m_global_offset, static_cast<long>(m_conf.file_size));
    } else {
        const auto ret = ::pread(m_open_files.back(), &m_last_file_data_end,
                                 sizeof(m_last_file_data_end), 0);
        if (ret != sizeof(m_last_file_data_end)) {
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "Could not read the data size");
        }
    }

    metric<storage_available_space_gauge, byte, int64_t>::
        register_gauge_callback(
            std::bind(&data_store::get_available_space, this));
    metric<storage_used_space_gauge, byte, int64_t>::register_gauge_callback(
        std::bind(&data_store::get_used_space, this));
    m_used = fetch_used_space();
}

address data_store::write(std::span<char> data) {

    if (m_used + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }

    auto alloc = allocate(static_cast<long>(data.size()));
    for (long written = 0; written < static_cast<long>(data.size());
         written += ::pwrite(alloc.fd, data.data() + written,
                             data.size() - written, alloc.seek + written))
        ;

    address data_address;
    data_address.push_fragment(
        {.pointer = alloc.global_offset, .size = data.size()});

    return data_address;
}

std::size_t data_store::read(char* buffer, uint128_t pointer, size_t size) {
    if (pointer < m_global_offset ||
        pointer + size > m_global_offset + m_used.load()) {
        throw std::out_of_range("pointer is out of range");
    }

    const auto [fd, seek] = get_file_offset_pair(pointer);

    ssize_t tr = 0;
    do {
        const auto r = ::pread(fd, buffer + tr, size - tr, seek + tr);
        if (r <= 0) [[unlikely]] {
            throw std::runtime_error(std::string("error in reading: ") +
                                     std::string(strerror(errno)));
        }
        tr += r;
    } while (static_cast<size_t>(tr) < size);

    return tr;
}

void data_store::sync() {

    std::unique_lock<std::mutex> lock(m_sync_end_offset_mutex);

    const auto ret = ::pwrite(m_open_files.back(), &m_last_file_data_end,
                              sizeof(m_last_file_data_end), 0);
    if (ret != sizeof(m_last_file_data_end)) [[unlikely]] {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Could not write the data size");
    }

    lock.unlock();
    fsync(m_open_files.back());
}

size_t data_store::fetch_used_space() const noexcept {
    const auto prev_files_data_size =
        m_conf.file_size * (m_open_files.size() - 1);
    return prev_files_data_size + m_last_file_data_end;
}

data_store::~data_store() {
    sync();
    for (const auto& open_file : m_open_files) {
        fsync(open_file);
        close(open_file);
    }
}

std::pair<int, long>
data_store::get_file_offset_pair(const uint128_t& pointer) const {

    const auto data_store_offset = (pointer - m_global_offset).get_low();
    const auto fd_index = data_store_offset / m_conf.file_size;
    const auto seek = data_store_offset - fd_index * m_conf.file_size;
    return {m_open_files.at(fd_index), seek};
}

int data_store::add_new_file(const uint128_t& offset, long file_size) {
    const auto file_path = m_conf.working_dir / get_name(offset);
    const int fd = open(file_path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd <= 0) [[unlikely]] {
        throw std::filesystem::filesystem_error(
            "Could not create new files in the data store root",
            std::error_code(errno, std::system_category()));
    }

    const int rc = ftruncate(fd, file_size);
    if (rc != 0) [[unlikely]] {
        throw std::filesystem::filesystem_error(
            "Could not truncate new files in the data store root",
            std::error_code(errno, std::system_category()));
    }

    m_last_file_data_end = sizeof(m_last_file_data_end);
    const auto ret =
        ::write(fd, &m_last_file_data_end, sizeof(m_last_file_data_end));
    if (ret != sizeof(m_last_file_data_end)) [[unlikely]] {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Could not write the data size");
    }
    m_open_files.emplace_back(fd);

    return fd;
}

std::pair<size_t, uint128_t>
data_store::parse_file_name(const std::string& filename) {
    const auto index1 = filename.find('_') + 1;
    const auto index2 = filename.substr(index1).find('_') + index1 + 1;
    const auto id_str = filename.substr(index1, index2 - 1 - index1);
    const auto offset_str = filename.substr(index2);
    return {std::stoul(id_str), big_int(offset_str)};
}

std::string data_store::get_name(const uint128_t& offset) const {
    return "data_" + std::to_string(m_data_id) + "_" + offset.to_string();
}

bool data_store::is_data_file(const std::filesystem::path& path) {
    return path.filename().string().starts_with("data_");
}

uint64_t data_store::get_used_space() const noexcept { return m_used; }

size_t data_store::get_available_space() const noexcept {
    auto space = std::filesystem::space(m_conf.working_dir);
    return std::min(space.available, m_conf.max_data_store_size - m_used);
}

data_store::alloc_t data_store::allocate(long size) {

    std::lock_guard<std::mutex> lock(m_allocate_mutex);

    if (m_last_file_data_end + size > m_conf.file_size) [[unlikely]] {
        sync();
        m_used += m_conf.file_size - m_last_file_data_end + sizeof (m_last_file_data_end);
        const auto new_offset =
            m_global_offset + m_conf.file_size * m_open_files.size();
        add_new_file(new_offset, m_conf.file_size);
    }

    alloc_t alloc;
    alloc.seek = m_last_file_data_end;
    alloc.fd = m_open_files.back();
    m_last_file_data_end = alloc.seek + size;
    alloc.global_offset = m_global_offset +
                          (m_open_files.size() - 1) * m_conf.file_size +
                          alloc.seek;

    m_used += size;

    return alloc;
}

} // end namespace uh::cluster
