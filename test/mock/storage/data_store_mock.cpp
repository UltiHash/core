
#include "data_store_mock.h"
#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"
#include "common/utils/pointer_traits.h"
#include <algorithm>
#include <fstream>

namespace uh::cluster {

struct ds_file_info {
    std::size_t storage_id;
    std::size_t offset;
};

data_store_mock::data_store_mock(data_store_config conf,
                                 const std::filesystem::path& working_dir,
                                 uint32_t service_id, uint32_t data_store_id)
    : m_storage_id(service_id),
      m_data_store_id(data_store_id),
      m_root(working_dir / std::to_string(data_store_id)),
      m_conf{conf} {

    m_data.reserve(m_conf.max_data_store_size);

    if (!std::filesystem::exists(m_root)) {
        std::filesystem::create_directories(m_root);
    }

    {
        std::ifstream ifs(m_root / m_datafile, std::ios::binary);
        if (ifs) {
            m_data.assign(std::istreambuf_iterator<char>(ifs),
                          std::istreambuf_iterator<char>());
        }
    }
    {
        std::ifstream ifs(m_root / m_refcountfile, std::ios::binary);
        if (ifs) {
            size_t map_size;
            ifs.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));

            for (size_t i = 0; i < map_size; ++i) {
                fragment key;

                ifs.read(reinterpret_cast<char*>(&key.pointer),
                         sizeof(key.pointer));
                ifs.read(reinterpret_cast<char*>(&key.size), sizeof(key.size));
                int value;
                ifs.read(reinterpret_cast<char*>(&value), sizeof(value));
                m_refcounter[key] = value;
            }
        }
    }
}

address data_store_mock::write(const std::string_view& data) {

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.size() + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.max_file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }

    address data_address;
    data_address.push({.pointer = pointer_traits::get_global_pointer(
                           m_data.size(), m_storage_id, m_data_store_id),
                       .size = data.size()});
    link(data_address);

    m_data.insert(m_data.end(), data.cbegin(), data.cend());

    return data_address;
}

void data_store_mock::manual_write(uint64_t internal_pointer,
                                   const std::string_view& data) {

    address data_address;
    data_address.push({.pointer = pointer_traits::get_global_pointer(
                           internal_pointer, m_storage_id, m_data_store_id),
                       .size = data.size()});
    link(data_address);

    m_data.insert(m_data.begin() + internal_pointer, data.cbegin(),
                  data.cend());
}

void data_store_mock::manual_read(uint64_t pointer, size_t size, char* buffer) {
    std::memcpy(buffer, m_data.data() + pointer, size);
}

std::size_t data_store_mock::read(char* buffer, const uint128_t& global_pointer,
                                  size_t size) {
    const auto pointer = pointer_traits::get_pointer(global_pointer);
    const auto current_offset = m_data.size();

    if (pointer_traits::get_service_id(global_pointer) != m_storage_id or
        pointer_traits::get_data_store_id(global_pointer) != m_data_store_id or
        pointer + size > current_offset) {
        LOG_WARN() << "attempted to read data from the out-of-bounds offset="
                   << pointer << ", with current_offset=" << current_offset;
        throw std::out_of_range("pointer is out of range");
    }

    std::memcpy(buffer, m_data.data() + pointer, size);
    return size;
}

std::size_t data_store_mock::read_up_to(
    char* buffer, const uh::cluster::uint128_t& global_pointer, size_t size) {

    const auto pointer = pointer_traits::get_pointer(global_pointer);
    const auto current_offset = m_data.size();

    size = std::min(size, current_offset - pointer);

    if (pointer_traits::get_service_id(global_pointer) != m_storage_id or
        pointer_traits::get_data_store_id(global_pointer) != m_data_store_id) {
        LOG_WARN() << "attempted to read data from the out-of-bounds offset="
                   << pointer << ", with current_offset=" << current_offset;
        throw std::out_of_range("pointer is out of range");
    }

    std::memcpy(buffer, m_data.data() + pointer, size);
    return size;
}

/**
 * @brief Creates a reference to one or multiple storage locations.
 * Invalid/non-existing fragments will be reported as rejected fragments
 * in a returned address.
 * @param address: storage locations that are to be referenced.
 * @return an address containing rejected fragments.
 */
address data_store_mock::link(const address& addr) {
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        m_refcounter[frag]++;
    }

    return addr;
}

/**
 * @brief Removes a reference to one or multiple storage locations.
 * If a storage location is no longer referenced, it is deleted and the
 * space it was using is made available for reuse.
 * @param address: storage locations that are to be unreferenced.
 * @return number of bytes freed in response to removing references.
 * In case of an error, std::numeric_limits<std::size_t>::max() is returned.
 */
size_t data_store_mock::unlink(const address& addr) {
    // auto addr.pointers
    size_t size = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        auto it = m_refcounter.find(frag);
        if (it == m_refcounter.end()) {
            throw std::exception();
            // return std::numeric_limits<std::size_t>::max();
        }
        if (--(it->second) == 0) {
            auto pointer = pointer_traits::get_pointer(frag.pointer);
            std::fill(m_data.begin() + pointer,
                      m_data.begin() + pointer + frag.size, 0);
            m_refcounter.erase(it);
            size += frag.size;
        }
    }
    return size;
}

uint64_t data_store_mock::get_used_space() const noexcept {
    return m_data.size();
}

size_t data_store_mock::get_available_space() const noexcept {
    return m_conf.max_data_store_size - m_data.size();
}

size_t data_store_mock::id() const noexcept { return m_data_store_id; }

data_store_mock::~data_store_mock() {
    {
        std::ofstream ofs(m_root / m_datafile, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
    }
    {
        std::ofstream ofs(m_root / m_refcountfile, std::ios::binary);
        size_t map_size = m_refcounter.size();
        ofs.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));

        for (const auto& [key, value] : m_refcounter) {
            ofs.write(reinterpret_cast<const char*>(&key.pointer),
                      sizeof(key.pointer));
            ofs.write(reinterpret_cast<const char*>(&key.size),
                      sizeof(key.size));
            ofs.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
    }
}

} // end namespace uh::cluster
