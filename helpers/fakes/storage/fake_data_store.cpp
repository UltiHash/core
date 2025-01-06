
#include "fake_data_store.h"
#include "common/telemetry/log.h"
#include "common/utils/pointer_traits.h"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace uh::cluster {

struct ds_file_info {
    std::size_t storage_id;
    std::size_t offset;
};

fake_data_store::fake_data_store(data_store_config conf,
                                 const std::filesystem::path& working_dir,
                                 uint32_t service_id, uint32_t data_store_id)
    : m_storage_id(service_id),
      m_data_store_id(data_store_id),
      m_root(working_dir / std::to_string(data_store_id)),
      m_conf{conf},
      m_data(m_conf.max_data_store_size, 0) {

    if (!std::filesystem::exists(m_root)) {
        std::filesystem::create_directories(m_root);
    }

    {
        std::ifstream ifs(m_root / m_datafile, std::ios::binary);
        if (ifs) {
            ifs.read(m_data.data(), m_data.size());

            std::streamsize bytes_read = ifs.gcount();
            if (bytes_read < 0) {
                throw std::runtime_error("Stream read error occurred.");
            }
            m_current_offset = static_cast<std::size_t>(bytes_read);
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

address fake_data_store::write(const std::string_view& data,
                               const std::vector<std::size_t>& offsets) {

    auto current_offset = m_current_offset.fetch_add(data.size());
    if (current_offset + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.max_file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }
    address data_address;
    data_address.push({.pointer = pointer_traits::get_global_pointer(
                           current_offset, m_storage_id, m_data_store_id),
                       .size = data.size()});

    std::copy(data.cbegin(), data.cend(), m_data.begin() + current_offset);
    link(data_address);
    return data_address;
}

void fake_data_store::manual_write(uint64_t internal_pointer,
                                   const std::string_view& data) {

    address data_address;
    data_address.push({.pointer = pointer_traits::get_global_pointer(
                           internal_pointer, m_storage_id, m_data_store_id),
                       .size = data.size()});
    link(data_address);

    std::copy(data.cbegin(), data.cend(), m_data.begin() + internal_pointer);
}

void fake_data_store::manual_read(uint64_t pointer, size_t size, char* buffer) {
    std::memcpy(buffer, m_data.data() + pointer, size);
}

std::size_t fake_data_store::read(char* buffer, const uint128_t& global_pointer,
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

    std::memcpy(buffer, m_data.data() + pointer, size);
    return size;
}

std::size_t fake_data_store::read_up_to(
    char* buffer, const uh::cluster::uint128_t& global_pointer, size_t size) {

    const auto pointer = pointer_traits::get_pointer(global_pointer);
    const auto current_offset = m_current_offset.load();

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

address fake_data_store::link(const address& addr) {
    address new_fragments;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        if (!m_refcounter.contains(frag)) {
            new_fragments.push(frag);
        }
        m_refcounter[frag]++;
    }

    return new_fragments;
}

size_t fake_data_store::unlink(const address& addr) {
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

uint64_t fake_data_store::get_used_space() const noexcept {
    return m_current_offset.load();
}

size_t fake_data_store::get_available_space() const noexcept {
    return m_conf.max_data_store_size - m_current_offset.load();
}

void fake_data_store::clear() {
    m_data.clear();
    m_refcounter.clear();
}

size_t fake_data_store::id() const noexcept { return m_data_store_id; }

fake_data_store::~fake_data_store() {
    {
        std::ofstream ofs(m_root / m_datafile, std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(m_data.data()),
                  m_current_offset.load());
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
