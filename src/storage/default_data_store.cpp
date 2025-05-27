#include "default_data_store.h"

#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/error.h>
#include <common/utils/io.h>
#include <common/utils/pointer_traits.h>

#include <mutex>
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
      m_file_count(m_files.size()),
      m_meta_fd(open_metadata(working_dir / std::string("ds.meta"))),
      m_used_space(fetch_used_space()),
      m_refcounter(
          m_root, m_conf.page_size,
          std::bind_front(&default_data_store::internal_delete, this)) {

    if (m_filesize % m_conf.page_size != 0) {
        throw std::runtime_error(
            "data store file size must be a multiple of ref-counter page size");
    }

    m_files.reserve(m_conf.max_data_store_size / m_filesize + 1);
    read_metadata();
}

std::size_t default_data_store::read(std::size_t local_pointer,
                                     std::span<char> buffer) {
    std::shared_lock lock(m_mutex);
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

void default_data_store::sync() {
    m_files.back().sync();

    write_metadata();
    int md_rv = fsync(m_meta_fd);
    if (md_rv == -1) {
        throw_from_errno("metadata file sync failed for " +
                         (m_root + ".ds_store").string());
    }
}

std::size_t default_data_store::fetch_used_space() const {
    std::unique_lock lock(m_mutex);
    return std::accumulate(
        m_files.begin(), m_files.end(), 0ull,
        [](auto acc, const auto& it) { return acc + it.used_space(); });
}

address default_data_store::write(const allocation_t allocation,
                                  std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {
    std::unique_lock lock(m_mutex);
    std::size_t local_pointer = allocation.offset;
    allocate_files(local_pointer, allocation.size);

    address rv;

    std::size_t written = 0ull;
    while (written < data.size()) {
        auto loc = file_location(local_pointer);
        std::size_t file_offset = local_pointer % m_filesize;
        auto count = loc.file.write(file_offset, data.subspan(written));
        if (count == 0) {
            break;
        }

        rv.emplace_back(local_pointer, count);

        local_pointer += count;
        written += count;
    }
    if (written != allocation.size) {
        throw std::runtime_error("could not complete buffer write");
    }

    m_used_space += allocation.size;
    sync();

    maintain_refcount(allocation.offset, allocation.size, offsets);
    return rv;
}

address default_data_store::link(const address& addr) {
    std::unique_lock lock(m_mutex);
    return m_refcounter.increment(addr);
}

size_t default_data_store::unlink(const address& addr) {
    std::unique_lock lock(m_mutex);
    return m_refcounter.decrement(addr);
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

    while (m_file_count < required_file_count) {
        m_files.emplace_back(
            data_file::create(m_root / base_name(m_files.size()), m_filesize));
        m_file_count = m_files.size();
    }
}

default_data_store::location default_data_store::file_location(size_t pointer) {

    auto index = pointer / m_filesize;
    auto offset = pointer % m_filesize;

    if (index >= m_file_count) {
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
    std::shared_lock lock(m_mutex);
    return m_write_offset;
}

void default_data_store::set_write_offset(std::size_t val) noexcept {
    std::shared_lock lock(m_mutex);
    m_write_offset = val;
}

allocation_t default_data_store::allocate(size_t size, std::size_t alignment) {
    std::unique_lock lock(m_mutex);

    if (m_conf.max_data_store_size - m_write_offset < size) {
        throw std::runtime_error("datastore cannot store additional " +
                                 std::to_string(size) + " bytes");
    }

    if (m_write_offset % alignment != 0) {
        m_write_offset += alignment - (m_write_offset % alignment);
    }

    allocation_t rv = {.offset = m_write_offset, .size = size};
    m_write_offset += size;
    return rv;
}

void default_data_store::maintain_refcount(
    const std::size_t local_pointer, const std::size_t size,
    const std::vector<std::size_t>& offsets) {
    std::deque<reference_counter::refcount_cmd> refcount_commands;

    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        std::size_t frag_size =
            std::next(it) == offsets.end() ? size - *it : *std::next(it) - *it;
        refcount_commands.emplace_back(reference_counter::INCREMENT,
                                       local_pointer + *it, frag_size);
    }

    update_last_page_ref(refcount_commands);
    m_refcounter.execute(refcount_commands);
}

void default_data_store::update_last_page_ref(
    std::deque<reference_counter::refcount_cmd>& refcount_commands) {

    std::size_t last_page = m_write_offset / m_conf.page_size;

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

std::size_t default_data_store::internal_delete(std::size_t offset,
                                                std::size_t size) {
    std::size_t last_page_offset =
        (m_write_offset / m_conf.page_size) * m_conf.page_size;

    if (offset >= last_page_offset) {
        LOG_WARN() << "attempted to delete data at the out-of-bounds offset="
                   << offset << ", with last_page_offset=" << last_page_offset;
        throw std::out_of_range("pointer for delete operation is out of range");
    }

    LOG_DEBUG() << "page " << offset / m_conf.page_size
                << " dropped to 0, deleting page (offset=" << offset
                << ", size=" << size << ")";

    std::size_t rv = 0ull;
    while (rv < size) {
        auto loc = file_location(offset + rv);
        auto count = loc.file.release(loc.offset, size - rv);
        if (count == 0) {
            break;
        }
        rv += count;
    }

    m_used_space -= rv;
    return rv;
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

} // end namespace uh::cluster
