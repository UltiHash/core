#include "default_data_store.h"

#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>
#include <common/utils/pointer_traits.h>

namespace uh::cluster {

namespace {

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
                       uint32_t service_id, uint32_t data_store_id)
    : m_storage_id(service_id),
      m_data_store_id(data_store_id),
      m_root(working_dir / std::to_string(data_store_id)),
      m_conf(conf),
      m_filesize(m_conf.max_file_size),
      m_files(load_files(m_root, m_filesize)),
      m_file_count(m_files.size()),
      m_used_space(fetch_used_space()),
      m_refcounter(m_root, m_conf.page_size,
                   std::bind_front(&default_data_store::internal_delete, this)) {

    if (m_filesize % m_conf.page_size != 0) {
        throw std::runtime_error(
            "data store file size must be a multiple of ref-counter page size");
    }

    m_files.reserve(m_conf.max_data_store_size / m_filesize + 1);

    metric<storage_available_space_gauge, byte,
           int64_t>::register_gauge_callback([this] {
        return get_available_space();
    });

    metric<storage_used_space_gauge, byte, int64_t>::register_gauge_callback(
        [this] { return get_used_space(); });
}

std::size_t default_data_store::read(char* buffer, const uint128_t& global_pointer,
                             size_t size) {
    // TODO what is the difference to read_up_to? Consult former implementation
    return read_up_to(buffer, global_pointer, size);
}

std::size_t default_data_store::read_up_to(char* buffer,
                                   const uh::cluster::uint128_t& global_pointer,
                                   size_t size) {
    std::size_t rv = 0ull;
    auto pointer = global_pointer;

    while (rv < size) {
        auto loc = file_location(pointer_traits::get_pointer(pointer));
        auto count =
            loc.file.read(loc.offset, std::span<char>{buffer + rv, size - rv});
        if (count == 0) {
            break;
        }

        pointer += count;
        rv += count;
    }

    return rv;
}

void default_data_store::sync() {
    std::unique_lock lock(m_mutex);
    m_files.back().sync();
}

std::size_t default_data_store::fetch_used_space() const {
    std::unique_lock lock(m_mutex);
    return std::accumulate(
        m_files.begin(), m_files.end(), 0ull,
        [](auto acc, const auto& it) { return acc + it.used_space(); });
}

address default_data_store::write(const std::string_view& data,
                          const std::vector<std::size_t>& offsets) {
    std::span<const char> span(data.data(), data.size());
    auto allocation = allocate(span.size(), offsets);

    address rv;

    std::size_t data_offs = 0ull;
    for (const auto& alloc : allocation) {
        auto count = alloc.l.file.write(alloc.l.offset,
                                        span.subspan(data_offs, alloc.size));

        if (count != alloc.size) {
            throw std::runtime_error("could not complete buffer write");
        }

        data_offs += count;
        auto frag = fragment{.pointer = pointer_traits::get_global_pointer(
                                 alloc.local, m_storage_id, m_data_store_id),
                             .size = alloc.size};
        rv.push(frag);
    }

    sync();
    return rv;
}

void default_data_store::manual_write(uint64_t pointer, const std::string_view& data) {

    auto loc = file_location(pointer);
    loc.file.write(loc.offset, std::span<const char>{data.data(), data.size()});
}

void default_data_store::manual_read(uint64_t pointer, size_t size, char* buffer) {

    auto loc = file_location(pointer);
    loc.file.read(loc.offset, std::span<char>{buffer, size});
}

address default_data_store::link(const address& addr) {
    std::unique_lock lock(m_mutex);
    return m_refcounter.increment(addr);
}

size_t default_data_store::unlink(const address& addr) {
    std::unique_lock lock(m_mutex);
    return m_refcounter.decrement(addr);
}

size_t default_data_store::id() const noexcept { return m_data_store_id; }

default_data_store::~default_data_store() {
    for (auto& file : m_files) {
        try {
            file.sync();
        } catch (const std::exception& e) {
            LOG_WARN() << "error syncing file: " << e.what();
        }
    }
}

default_data_store::location default_data_store::file_location(size_t pointer) {

    auto index = pointer / m_filesize;
    auto offset = pointer % m_filesize;

    auto size = m_file_count.load();
    if (index >= size) {
        throw std::out_of_range("pointer out of range");
    }

    return location{.file = m_files[index], .offset = offset};
}

uint64_t default_data_store::get_used_space() const noexcept {
    return m_used_space;
}

size_t default_data_store::get_available_space() const noexcept {
    auto capacity = m_conf.max_data_store_size - m_used_space;
    try {
        auto space = std::filesystem::space(m_root);
        capacity = std::min(space.available, capacity);
    } catch (...) {
    }

    return capacity;
}

std::list<default_data_store::alloc_t>
default_data_store::allocate(size_t size, const std::vector<std::size_t>& offsets) {

    std::unique_lock lock(m_mutex);

    if (m_conf.max_data_store_size - m_files.size() * m_filesize +
            m_files.back().free() <
        size) {
        throw std::runtime_error("datastore cannot store additional " +
                                 std::to_string(size) + " bytes");
    }

    std::list<alloc_t> rv;
    std::size_t allocated = 0ull;

    while (size > allocated) {
        auto& file = m_files.back();

        std::size_t bytes = std::min(file.free(), size - allocated);
        std::size_t offset = file.alloc(bytes);

        rv.push_back(
            alloc_t{.l = {.file = file, .offset = offset},
                    .size = bytes,
                    .local = (m_files.size() - 1) * m_filesize + offset});

        allocated += bytes;

        if (file.free() == 0ull) {
            m_files.back().sync();
            m_files.emplace_back(data_file::create(
                m_root / base_name(m_files.size()), m_filesize));
            m_file_count = m_files.size();
        }
    }

    m_used_space += allocated;

    if (rv.empty()) {
        return rv;
    }

    std::size_t local_pointer = rv.front().local;

    std::deque<reference_counter::refcount_cmd> refcount_commands;

    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        std::size_t frag_size =
            std::next(it) == offsets.end() ? size - *it : *std::next(it) - *it;
        refcount_commands.emplace_back(reference_counter::INCREMENT,
                                       local_pointer + *it, frag_size);
    }

    update_last_page_ref(refcount_commands);
    m_refcounter.execute(refcount_commands);

    return rv;
}

void default_data_store::update_last_page_ref(
    std::deque<reference_counter::refcount_cmd>& refcount_commands) {

    std::size_t current_offset =
        m_files.size() * m_filesize - m_files.back().free();
    std::size_t last_page = current_offset / m_conf.page_size;

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

std::size_t default_data_store::internal_delete(std::size_t offset, std::size_t size) {

    std::size_t file_count = m_file_count;
    std::size_t curr_offs =
        file_count * m_filesize - m_files[file_count - 1].free();

    std::size_t last_page_offset =
        (curr_offs / m_conf.page_size) * m_conf.page_size;

    if (offset >= last_page_offset) {
        LOG_WARN() << "attempted to delete data at the out-of-bounds offset="
                   << offset << ", with last_page_offset=" << last_page_offset;
        throw std::out_of_range("pointer for delete operation is out of range");
    }

    LOG_DEBUG() << "page " << offset / m_conf.page_size
                << " dropped to 0, deleting page (offset=" << offset
                << ", size=" << size << ")";

    auto loc = file_location(offset);
    loc.file.release(loc.offset, size);

    m_used_space -= size;
    return size;
}

} // end namespace uh::cluster
