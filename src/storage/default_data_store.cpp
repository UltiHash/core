#include "default_data_store.h"

#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/error.h>
#include <common/utils/io.h>
#include <common/utils/pointer_traits.h>

#include <set>

namespace uh::cluster {

namespace {

struct metadata {
    std::size_t write_offset = 0ull;
    std::size_t used_space = 0ull;
};

std::string base_name(std::size_t number) {
    std::stringstream s;
    s << "data_" << std::setfill('0') << std::setw(6) << number;

    return s.str();
}

std::vector<data_file> load_files(const std::filesystem::path& root,
                                  std::size_t filesize) {

    if (!std::filesystem::exists(root)) {
        if (!std::filesystem::create_directories(root)) {
            throw std::runtime_error("could not create data_store directory " +
                                     root.string());
        }
    }

    std::set<std::filesystem::path> files;
    for (auto entry : std::filesystem::directory_iterator(root)) {
        auto path = entry.path();
        if (path.extension() != data_file::EXTENSION_DATA_FILE) {
            continue;
        }

        files.insert(path.replace_extension());
    }

    std::vector<data_file> rv;
    for (const auto& root : files) {
        rv.emplace_back(root);
    }

    if (rv.empty()) {
        rv.emplace_back(data_file::create(root / base_name(0), filesize));
    }

    return rv;
}

} // namespace

default_data_store::default_data_store(data_store_config conf,
                                       const std::filesystem::path& working_dir,
                                       uint32_t service_id)
    : m_storage_id(service_id),
      m_root(working_dir),
      m_conf(conf),
      m_filesize(m_conf.max_file_size),
      m_files(load_files(m_root, m_filesize)),
      m_meta_fd(open_metadata(working_dir / std::string("ds.meta"))),
      m_used_space(fetch_used_space()),
      m_refcounter(
          m_root, m_conf.page_size,
          std::bind_front(&default_data_store::internal_delete, this)) {

    (void)m_storage_id;
    if (m_filesize % m_conf.page_size != 0) {
        throw std::runtime_error(
            "data store file size must be a multiple of ref-counter page size");
    }

    m_files.reserve(m_conf.max_data_store_size / m_filesize + 1);
    read_metadata();
}

std::size_t default_data_store::read(std::size_t local_pointer,
                                     std::span<char> buffer) {
    std::size_t rv = 0ull;

    while (rv < buffer.size()) {
        auto loc = file_location(local_pointer);
        auto count = loc.file.read(loc.offset, buffer.subspan(rv));
        if (count == 0) {
            break;
        }

        local_pointer += count;
        rv += count;
    }

    return rv;
}

void default_data_store::sync(
    std::vector<std::reference_wrapper<data_file>> dirty_files) {
    for (auto file : dirty_files) {
        file.get().sync();
    }

    write_metadata();
    int md_rv = fsync(m_meta_fd);
    if (md_rv == -1) {
        throw_from_errno("metadata file sync failed for " +
                         (m_root + ".ds_store").string());
    }
}

std::size_t default_data_store::fetch_used_space() const {
    std::unique_lock lock(m_file_mutex);
    return std::accumulate(
        m_files.begin(), m_files.end(), 0ull,
        [](auto acc, const auto& it) { return acc + it.used_space(); });
}

void default_data_store::write(
    allocation_t allocation, const std::vector<std::span<const char>>& buffers,
    const std::vector<refcount_t>& refcounts) {
    std::size_t local_pointer = allocation.offset;
    allocate_files(local_pointer, allocation.size);

    auto size_sum =
        std::accumulate(buffers.begin(), buffers.end(), 0ul,
                        [](auto acc, const auto& v) { return acc + v.size(); });
    if (size_sum != allocation.size) {
        throw std::runtime_error("data is shorter than allocation size: " +
                                 std::to_string(size_sum) + " vs " +
                                 std::to_string(allocation.size));
    }

    std::vector<std::reference_wrapper<data_file>> dirty_files;
    for (const auto& data : buffers) {
        std::size_t written = 0ull;
        while (written < data.size()) {
            auto loc = file_location(local_pointer);
            std::size_t file_offset = local_pointer % m_filesize;
            auto count = loc.file.write(file_offset, data.subspan(written));
            if (count == 0) {
                break;
            }
            dirty_files.emplace_back(loc.file);

            local_pointer += count;
            written += count;
        }
        if (written != data.size()) {
            throw std::runtime_error("could not complete buffer write");
        }
    }

    m_used_space.fetch_add(allocation.size);
    std::size_t current = m_write_offset.load();
    std::size_t desired = allocation.offset + allocation.size;
    while (desired > current &&
           !m_write_offset.compare_exchange_weak(current, desired)) {
        desired = std::max(desired, current);
    }
    sync(dirty_files);

    m_refcounter.increment(refcounts, false);
}

std::vector<refcount_t>
default_data_store::link(const std::vector<refcount_t>& refcounts) {
    return m_refcounter.increment(refcounts);
}

std::size_t
default_data_store::unlink(const std::vector<refcount_t>& refcounts) {
    return m_refcounter.decrement(refcounts);
}

default_data_store::~default_data_store() {
    for (auto& file : m_files) {
        try {
            file.sync();
        } catch (const std::exception& e) {
            LOG_WARN() << "error syncing file: " << e.what();
        }
    }

    if (m_meta_fd != -1) {
        if (close(m_meta_fd) == -1) {
            LOG_WARN() << "error closing file descriptor: " << errno_message();
        }
    }
}

void default_data_store::allocate_files(std::size_t offset, std::size_t size) {
    auto required_file_count = ((offset + size) / m_filesize) + 1;

    if (required_file_count <= m_file_count) {
        return;
    }

    std::unique_lock lock(m_file_mutex);
    while (m_file_count < required_file_count) {
        m_files.emplace_back(
            data_file::create(m_root / base_name(m_files.size()), m_filesize));
        m_file_count = m_files.size();
    }
}

default_data_store::location default_data_store::file_location(size_t pointer) {

    auto index = pointer / m_filesize;
    auto offset = pointer % m_filesize;

    if (index >= m_files.size()) {
        throw std::out_of_range("pointer out of range");
    }

    return location{.file = m_files[index], .offset = offset};
}

std::size_t default_data_store::get_used_space() const noexcept {
    return m_used_space;
}

std::size_t default_data_store::get_available_space() const noexcept {
    auto capacity = m_conf.max_data_store_size - m_used_space;
    try {
        auto space = std::filesystem::space(m_root);
        capacity = std::min(space.available, capacity);
    } catch (...) {
    }

    return capacity;
}

std::size_t default_data_store::get_write_offset() const noexcept {
    return m_write_offset;
}

void default_data_store::set_write_offset(std::size_t val) noexcept {
    m_write_offset = val;
}

allocation_t default_data_store::allocate(size_t size, std::size_t alignment) {
    if (alignment == 0) {
        throw std::invalid_argument("alignment must be larger than zero");
    }

    if (size == 0) {
        throw std::invalid_argument("allocation size must be larger than zero");
    }

    std::size_t current_offset = m_write_offset.load();
    std::size_t new_offset;
    std::size_t allocation_start;

    do {
        allocation_start = current_offset;

        if (allocation_start % alignment != 0) {
            allocation_start += alignment - (allocation_start % alignment);
        }

        if (m_conf.max_data_store_size - allocation_start < size) {
            throw std::runtime_error("datastore cannot store additional " +
                                     std::to_string(size) + " bytes");
        }

        new_offset = allocation_start + size;

    } while (!m_write_offset.compare_exchange_weak(current_offset, new_offset));

    return {.offset = allocation_start, .size = size};
}

std::size_t default_data_store::internal_delete(std::size_t offset,
                                                std::size_t size) {
    std::size_t current_write_offset = m_write_offset.load();
    if (offset >= current_write_offset) {
        LOG_WARN() << "attempted to delete data at the out-of-bounds offset="
                   << offset
                   << ", with m_write_offset=" << current_write_offset;
        throw std::out_of_range("pointer for delete operation is out of range");
    }

    std::size_t adjusted_size = std::min(size, current_write_offset - offset);

    LOG_DEBUG() << "page " << offset / m_conf.page_size
                << " dropped to 0, deleting page (offset=" << offset
                << ", size=" << adjusted_size << ")";

    std::size_t bytes_released = 0ull;
    while (bytes_released < adjusted_size) {
        // it seems pointer of of range is coming from here
        auto loc = file_location(offset + bytes_released);
        auto count =
            loc.file.release(loc.offset, adjusted_size - bytes_released);
        if (count == 0) {
            break;
        }
        bytes_released += count;
    }

    m_used_space -= bytes_released;

    return bytes_released;
}

int default_data_store::open_metadata(const std::filesystem::path& path) {
    bool file_existed = std::filesystem::exists(path);
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        throw_from_errno("could not open file " + path.string());
    }

    if (!file_existed) {
        metadata md{.write_offset = 0ull, .used_space = 0ull};
        safe_pwrite(fd,
                    std::span<const char>(reinterpret_cast<const char*>(&md),
                                          sizeof(metadata)),
                    0);
    } else {
        auto size = std::filesystem::file_size(path);
        if (size != sizeof(metadata)) {
            throw std::runtime_error("metadata file has invalid size");
        }
    }

    return fd;
}

void default_data_store::read_metadata() {
    metadata md;

    safe_pread(m_meta_fd,
               std::span<char>(reinterpret_cast<char*>(&md), sizeof(metadata)),
               0);

    m_write_offset = md.write_offset;
    m_used_space = md.used_space;
}

void default_data_store::write_metadata() {
    metadata md{.write_offset = m_write_offset, .used_space = m_used_space};

    safe_pwrite(m_meta_fd,
                std::span<const char>(reinterpret_cast<const char*>(&md),
                                      sizeof(metadata)),
                0);
}
std::vector<refcount_t>
default_data_store::get_refcounts(const std::vector<std::size_t>& stripe_ids) {
    return m_refcounter.get_refcounts(stripe_ids);
}

} // end namespace uh::cluster
