#include "data_store.h"
#include "common/telemetry/metrics.h"
#include "common/utils/pointer_traits.h"
#include <mutex>

namespace uh::cluster {

data_store::data_store(data_store_config conf, uint32_t service_id, uint32_t data_store_id)
    : m_storage_id (service_id),
      m_data_store_id (data_store_id),
      m_root (conf.working_dir / std::to_string (data_store_id)),
      m_conf(std::move(conf)) {

    m_open_files.reserve(2 * m_conf.max_data_store_size / m_conf.file_size + 1);

    if (!std::filesystem::exists(m_root)) {
        std::filesystem::create_directories(m_root);
    }

    std::map<uint128_t, int> open_files;
    for (const auto& entry :
         std::filesystem::directory_iterator(m_root)) {
        if (!is_data_file(entry.path())) {
            continue;
        }

        const auto id_offset = parse_file_name(entry.path().filename());
        if (id_offset.first != m_storage_id) [[unlikely]] { // non-adaptive data store with fixed id
            throw std::range_error(
                "The data store is spawned on the wrong data node");
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
        add_new_file(0, static_cast<long>(m_conf.file_size));
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

std::size_t data_store::read(char* buffer, const uint128_t& global_pointer, size_t size) {
    const auto pointer = pointer_traits::get_pointer(global_pointer);
    if (pointer_traits::get_service_id(global_pointer) != m_storage_id or pointer + size >  m_used.load()) {
        throw std::out_of_range("pointer is out of range");
    }

    std::unique_lock <std::mutex> lk (m_async_mutex);
    if (const auto [async_offset, data] = find_async_data(pointer, size); data.data() != nullptr) {
        const auto offset = pointer - async_offset;
        std::memcpy(buffer, data.data() + offset, size);
        return size;
    }
    lk.unlock ();

    const auto [fd, seek] = get_file_offset_pair(pointer_traits::get_pointer(global_pointer));

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


    fdatasync(m_open_files.back());
}

size_t data_store::fetch_used_space() const noexcept {
    const auto prev_files_data_size =
        m_conf.file_size * (m_open_files.size() - 1);
    return prev_files_data_size + m_last_file_data_end;
}

address data_store::register_write(const shared_buffer<char>& data) {
    if (m_used + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }
    auto alloc = internal_allocate(data.size());
    address data_address;
    data_address.push_fragment(
        {.pointer = alloc.global_offset, .size = data.size()});

    std::lock_guard <std::mutex> lk (m_async_mutex);
    m_ongoing_async_writes.emplace(pointer_traits::get_pointer(alloc.global_offset), std::make_pair(alloc, data));
    return data_address;
}

address data_store::register_write(const std::string_view& data) {
    shared_buffer <char> buffer (data.size());
    std::memcpy(buffer.data(), data.data(), data.size());
    return register_write(buffer);
}

void data_store::perform_write(const address& addr) {
    if (addr.size() != 1) {
        throw std::runtime_error ("Invalid address size");
    }

    const auto pointer = pointer_traits::get_pointer(addr.first().pointer);
    std::unique_lock <std::mutex> lk (m_async_mutex);
    auto& [alloc, data] = m_ongoing_async_writes.at (pointer);
    lk.unlock();

    for (long written = 0; written < static_cast<long>(data.size());
         written += ::pwrite(alloc.fd, data.data() + written,
                             data.size() - written, alloc.seek + written))
        ;
    std::lock_guard <std::mutex> rm_lk (m_async_mutex);
    m_ongoing_async_writes.erase(pointer);
    m_async_cv.notify_all();
}

void data_store::wait_for_ongoing_writes(const address& addr) {
    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get_fragment(i);
        const auto pointer = pointer_traits::get_pointer(frag.pointer);

        std::unique_lock <std::mutex> lk (m_async_mutex);
        m_async_cv.wait(lk, [this, pointer, size=frag.size] () {
            return find_async_data(pointer, size).first == 0;
        });
    }
}

data_store::~data_store() {
    sync();
    for (const auto& open_file : m_open_files) {
        fsync(open_file);
        close(open_file);
    }
}

std::pair<int, long>
data_store::get_file_offset_pair(size_t pointer) const {
    const auto fd_index = pointer / m_conf.file_size;
    const auto seek = pointer - fd_index * m_conf.file_size;
    return {m_open_files.at(fd_index), seek};
}

int data_store::add_new_file(size_t offset, long file_size) {
    const auto file_path = m_root / get_name(offset);
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

std::pair<size_t, size_t>
data_store::parse_file_name(const std::string& filename) {
    const auto index1 = filename.find_first_of('_') + 1;
    const auto index2 = filename.substr(index1).find('_') + index1 + 1;
    const auto id_str = filename.substr(index1, index2 - 1 - index1);
    const auto offset_str = filename.substr(index2);
    return {std::stoul(id_str), std::stoul(offset_str)};
}

std::string data_store::get_name(size_t offset) const {
    return "data_" + std::to_string(m_storage_id) + "_" + std::to_string(offset);
}

bool data_store::is_data_file(const std::filesystem::path& path) {
    return path.filename().string().starts_with("data_");
}

uint64_t data_store::get_used_space() const noexcept { return m_used; }

size_t data_store::get_available_space() const noexcept {
    auto space = std::filesystem::space(m_root);
    return std::min(space.available, m_conf.max_data_store_size - m_used);
}

data_store::alloc_t data_store::internal_allocate(long size) {

    std::lock_guard<std::mutex> lock(m_allocate_mutex);

    if (m_last_file_data_end + size > m_conf.file_size) [[unlikely]] {
        sync();
        m_used += m_conf.file_size - m_last_file_data_end + sizeof (m_last_file_data_end);
        add_new_file(m_conf.file_size * m_open_files.size(), m_conf.file_size);
    }

    alloc_t alloc;
    alloc.seek = m_last_file_data_end;
    alloc.fd = m_open_files.back();
    m_last_file_data_end = alloc.seek + size;
    alloc.global_offset = pointer_traits::get_global_pointer((m_open_files.size() - 1) * m_conf.file_size +
                          alloc.seek, m_storage_id, m_data_store_id);

    m_used += size;

    return alloc;
}

std::pair<size_t, shared_buffer<char>>
data_store::find_async_data(size_t pointer, size_t size) {
    auto async_data = m_ongoing_async_writes.upper_bound(pointer);
    if (async_data != m_ongoing_async_writes.cbegin()) {
        async_data --;
        if (async_data->first + async_data->second.second.size() >= pointer + size) {
            return {async_data->first, async_data->second.second};
        }
    }
    return {0, nullptr};
}
} // end namespace uh::cluster
