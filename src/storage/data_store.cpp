#include "data_store.h"
#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"
#include "common/utils/pointer_traits.h"
#include <mutex>

namespace uh::cluster {

struct ds_file_info {
    std::size_t storage_id;
    std::size_t offset;
};

ds_file_info parse_file_name(const std::string& filename) {
    const auto index1 = filename.find_first_of('_') + 1;
    const auto index2 = filename.substr(index1).find('_') + index1 + 1;
    const auto id_str = filename.substr(index1, index2 - 1 - index1);
    const auto offset_str = filename.substr(index2);
    return {std::stoul(id_str), std::stoul(offset_str)};
}

data_store::data_store(data_store_config conf,
                       const std::filesystem::path& working_dir,
                       uint32_t service_id, uint32_t data_store_id)
    : m_storage_id(service_id),
      m_data_store_id(data_store_id),
      m_root(working_dir / std::to_string(data_store_id)),
      m_conf(conf),
      m_refcounter(m_root, m_conf.page_size,
                   std::bind_front(&data_store::internal_delete, this)) {

    m_open_files.reserve(2 * m_conf.max_data_store_size / m_conf.max_file_size +
                         1);

    if (!std::filesystem::exists(m_root)) {
        std::filesystem::create_directories(m_root);
    }

    std::map<std::size_t, std::pair<int, std::filesystem::path>> open_files;
    for (const auto& entry : std::filesystem::directory_iterator(m_root)) {
        if (!is_data_file(entry.path())) {
            continue;
        }

        const auto file_info = parse_file_name(entry.path().filename());
        if (file_info.storage_id != m_storage_id)
            [[unlikely]] { // non-adaptive data store with fixed id
            throw std::range_error(
                "The data store is spawned on the wrong storage service");
        }

        const int fd = open(entry.path().c_str(), O_RDWR);
        if (fd <= 0) [[unlikely]] {
            throw std::filesystem::filesystem_error(
                "Could not open the files in the data store root",
                std::error_code(errno, std::system_category()));
        }

        open_files.emplace(file_info.offset, std::pair{fd, entry.path()});
    }

    for (const auto& of : open_files) {
        m_open_files.emplace_back(of.second.first, of.first);
    }

    std::filesystem::path last_path;

    if (m_open_files.empty()) {
        last_path = add_new_file(0, static_cast<long>(m_conf.max_file_size));
    } else {
        const auto ret =
            ::pread(m_open_files.back().first, &m_last_file_data_end,
                    sizeof(m_last_file_data_end), m_conf.max_file_size);
        if (ret != sizeof(m_last_file_data_end)) {
            throw std::system_error(
                std::error_code(errno, std::system_category()),
                "Could not read the data size");
        }
        last_path = open_files.crbegin()->second.second;
    }

    metric<storage_available_space_gauge, byte,
           int64_t>::register_gauge_callback([this] {
        return get_available_space();
    });
    metric<storage_used_space_gauge, byte, int64_t>::register_gauge_callback(
        [this] { return get_used_space(); });

    m_current_offset = fetch_used_space(last_path);
    // TODO: calculated space on startup may not consider punched holes
    m_used_space = fetch_used_space(last_path);
    LOG_DEBUG() << "data store " << m_data_store_id
                << ": current_offset=" << m_current_offset;
}

std::size_t data_store::read(char* buffer, const uint128_t& global_pointer,
                             size_t size) {
    const auto pointer = pointer_traits::get_pointer(global_pointer);
    const auto current_offset = m_current_offset.load();

    if (pointer_traits::get_service_id(global_pointer) != m_storage_id or
        pointer_traits::get_data_store_id(global_pointer) != m_data_store_id or
        pointer + size > current_offset) {
        LOG_WARN() << "attempted to read data from the out-of-bounds offset="
                   << pointer << ", with current_offset=" << current_offset;
        throw std::out_of_range("pointer is out of range");
    }

    const auto [fd, seek] =
        get_file_offset_pair(pointer_traits::get_pointer(global_pointer));

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

std::size_t data_store::read_up_to(char* buffer,
                                   const uh::cluster::uint128_t& global_pointer,
                                   size_t size) {
    const auto pointer = pointer_traits::get_pointer(global_pointer);
    const auto current_offset = m_current_offset.load();

    if (pointer_traits::get_service_id(global_pointer) != m_storage_id or
        pointer_traits::get_data_store_id(global_pointer) != m_data_store_id or
        pointer > current_offset) {
        LOG_WARN() << "attempted to read data from the out-of-bounds offset="
                   << pointer << ", with current_offset=" << current_offset;
        throw std::out_of_range("pointer is out of range");
    }

    const auto [fd, seek] =
        get_file_offset_pair(pointer_traits::get_pointer(global_pointer));

    ssize_t tr = 0;
    auto max_size = size;
    if (const auto remaining_in_last_file = m_last_file_data_end - seek;
        fd == m_open_files.back().first and remaining_in_last_file < size) {
        max_size = remaining_in_last_file;
    }

    while (static_cast<size_t>(tr) < max_size) {
        const auto r = ::pread(fd, buffer + tr, max_size - tr, seek + tr);
        if (r == 0) [[unlikely]] {
            return tr;
        }
        if (r < 0) [[unlikely]] {
            throw std::runtime_error(std::string("error in reading: ") +
                                     std::string(strerror(errno)));
        } else
            tr += r;
    }

    return tr;
}

void data_store::sync() {

    std::unique_lock<std::mutex> lock(m_sync_mutex);

    const auto ret =
        ::pwrite(m_open_files.back().first, &m_last_file_data_end,
                 sizeof(m_last_file_data_end), m_conf.max_file_size);
    if (ret != sizeof(m_last_file_data_end)) [[unlikely]] {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Could not write the data size");
    }

    lock.unlock();

    fdatasync(m_open_files.back().first);
}

size_t data_store::fetch_used_space(
    const std::filesystem::path& last_file) const noexcept {
    auto size = 0ul;
    for (auto& f : std::filesystem::recursive_directory_iterator(m_root)) {
        if (!is_data_file(f.path())) {
            continue;
        }
        if (last_file != f.path())
            size += std::filesystem::file_size(f.path());
    }
    return size + m_last_file_data_end;
}

address data_store::write(const std::string_view& data,
                          const std::vector<std::size_t>& offsets) {
    if (m_current_offset + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.max_file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }

    auto alloc = internal_allocate(data.size(), offsets);

    long written = 0;
    while (written < static_cast<long>(data.size())) {
        auto size =
            ::pwrite(alloc.fd, data.data() + written, data.size() - written,
                     static_cast<long>(alloc.seek) + written);
        if (size == 0) {
            throw std::runtime_error("Could not perform write");
            // TODO: roll back ref counter increment
        }
        written += size;
    }
    sync();

    address data_address;
    data_address.push({.pointer = alloc.global_offset, .size = data.size()});
    return data_address;
}

void data_store::manual_write(uint64_t internal_pointer,
                              const std::string_view& data) {

    const auto [fd, seek] = get_file_offset_pair(internal_pointer);

    long written = 0;
    while (written < static_cast<long>(data.size())) {
        auto size = ::pwrite(fd, data.data() + written, data.size() - written,
                             seek + written);
        if (size == 0) {
            throw std::runtime_error("Could not perform write");
        }
        written += size;
    }
}

void data_store::manual_read(uint64_t pointer, size_t size, char* buffer) {
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
}

address data_store::link(const address& addr) {
    return m_refcounter.increment(addr);
}

size_t data_store::unlink(const address& addr) {
    return m_refcounter.decrement(addr);
}

size_t data_store::id() const noexcept { return m_data_store_id; }

data_store::~data_store() {
    sync();
    for (const auto& open_file : m_open_files) {
        fsync(open_file.first);
        close(open_file.first);
    }
}

std::pair<int, long> data_store::get_file_offset_pair(size_t pointer) const {
    auto f = std::upper_bound(
        m_open_files.cbegin(), m_open_files.cend(), std::pair{0, pointer},
        [](const auto& v1, const auto& v2) { return v1.second < v2.second; });
    if (f == m_open_files.cbegin()) {
        throw std::out_of_range("pointer out of range");
    }
    f--;
    const auto seek = pointer - f->second;
    return {f->first, seek};
}

std::filesystem::path data_store::add_new_file(size_t offset,
                                               size_t file_size) {
    const auto file_path = m_root / get_name(offset);
    const int fd = open(file_path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd <= 0) [[unlikely]] {
        throw std::filesystem::filesystem_error(
            "Could not create new files in the data store root",
            std::error_code(errno, std::system_category()));
    }

    int rc = ftruncate(fd, file_size);
    if (rc != 0) [[unlikely]] {
        throw std::filesystem::filesystem_error(
            "Could not truncate new files in the data store root",
            std::error_code(errno, std::system_category()));
    }

    if (!m_open_files.empty()) {
        rc = ftruncate(m_open_files.back().first,
                       static_cast<long>(m_last_file_data_end));
        if (rc != 0) [[unlikely]] {
            throw std::filesystem::filesystem_error(
                "Could not truncate the last file in the data store root",
                std::error_code(errno, std::system_category()));
        }
    }

    m_last_file_data_end = 0;
    const auto ret =
        ::pwrite(fd, &m_last_file_data_end, sizeof(m_last_file_data_end),
                 m_conf.max_file_size);
    if (ret != sizeof(m_last_file_data_end)) [[unlikely]] {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Could not write the data size");
    }
    m_open_files.emplace_back(fd, pointer_traits::get_pointer(offset));

    return file_path;
}

std::string data_store::get_name(size_t offset) const {
    return "data_" + std::to_string(m_storage_id) + "_" +
           std::to_string(offset);
}

bool data_store::is_data_file(const std::filesystem::path& path) {
    return path.filename().string().starts_with("data_");
}

uint64_t data_store::get_used_space() const noexcept { return m_used_space; }

size_t data_store::get_available_space() const noexcept {
    auto capacity = m_conf.max_data_store_size - m_used_space;
    try {
        auto space = std::filesystem::space(m_root);
        capacity = std::min(space.available, capacity);
    } catch (...) {
    }

    return capacity;
}

data_store::alloc_t
data_store::internal_allocate(size_t size,
                              const std::vector<std::size_t>& offsets) {
    std::deque<reference_counter::refcount_cmd> refcount_commands;
    std::unique_lock<std::mutex> lock(m_allocate_mutex);

    if (m_last_file_data_end + size > m_conf.max_file_size) [[unlikely]] {
        const auto offset = m_open_files.back().second + m_last_file_data_end;
        add_new_file(offset, m_conf.max_file_size);
        assert(offset == m_current_offset);
    }

    alloc_t alloc;
    alloc.seek = m_last_file_data_end;
    alloc.fd = m_open_files.back().first;
    std::size_t local_pointer = m_open_files.back().second + alloc.seek;
    alloc.global_offset = pointer_traits::get_global_pointer(
        local_pointer, m_storage_id, m_data_store_id);

    m_last_file_data_end += size;
    m_current_offset += size;
    m_used_space += size;

    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        std::size_t frag_size =
            std::next(it) == offsets.end() ? size - *it : *std::next(it) - *it;
        refcount_commands.emplace_back(reference_counter::INCREMENT,
                                       local_pointer + *it, frag_size);
    }
    update_last_page_ref(refcount_commands);
    lock.unlock();

    m_refcounter.execute(refcount_commands);

    return alloc;
}

void data_store::update_last_page_ref(
    std::deque<reference_counter::refcount_cmd>& refcount_commands) {
    std::size_t last_page = m_current_offset / m_conf.page_size;
    if (m_locked_page.has_value()) {
        if (last_page != m_locked_page.value()) {
            refcount_commands.emplace_back(reference_counter::INCREMENT,
                                           last_page * m_conf.page_size,
                                           m_conf.page_size);
            refcount_commands.emplace_back(
                reference_counter::DECREMENT,
                m_locked_page.value() * m_conf.page_size, m_conf.page_size);
            m_locked_page = last_page;
        }
    } else {
        refcount_commands.emplace_back(reference_counter::INCREMENT,
                                       last_page * m_conf.page_size,
                                       m_conf.page_size);
        m_locked_page = last_page;
    }
}

std::size_t data_store::internal_delete(std::size_t offset, std::size_t size) {
    std::size_t last_page_id = m_current_offset.load() / m_conf.page_size;
    std::size_t last_page_offset = last_page_id * m_conf.page_size;
    if (offset >= last_page_offset) {
        LOG_WARN() << "attempted to delete data at the out-of-bounds offset="
                   << offset << ", with last_page_offset=" << last_page_offset;
        throw std::out_of_range("pointer for delete operation is out of range");
    }

    LOG_DEBUG() << "page " << offset / m_conf.page_size
                << " dropped to 0, deleting page (offset=" << offset
                << ", size=" << size << ")";

    const auto [fd, seek] = get_file_offset_pair(offset);

    if (fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, seek, size))
        [[unlikely]] {
        throw std::system_error(std::error_code(errno, std::system_category()),
                                "Could not deallocate the data.");
    }
    m_used_space -= size;
    return size;
}

} // end namespace uh::cluster
