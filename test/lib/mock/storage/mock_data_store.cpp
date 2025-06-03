#include "mock_data_store.h"
#include "common/telemetry/log.h"
#include "common/utils/pointer_traits.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>

namespace uh::cluster {

struct ds_file_info {
    std::size_t storage_id;
    std::size_t offset;
};

mock_data_store::mock_data_store(data_store_config conf,
                                 const std::filesystem::path& working_dir,
                                 uint32_t service_id, uint32_t data_store_id)
    : m_storage_id(service_id),
      m_data_store_id(data_store_id),
      m_root(working_dir / std::to_string(data_store_id)),
      m_conf{conf},
      m_data(m_conf.max_data_store_size, 0) {

    assert(m_data_store_id == 0);
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

address mock_data_store::write(const allocation_t allocation,
                               std::span<const char> data,
                               const std::vector<std::size_t>& offsets) {
    std::copy(data.begin(), data.end(), m_data.begin() + allocation.offset);

    address data_address;
    data_address.emplace_back(allocation.offset, data.size());
    link(data_address);
    return data_address;
}

std::size_t mock_data_store::read(const std::size_t pointer,
                                  std::span<char> buffer) {
    const auto current_offset = m_current_offset.load();

    if (pointer + buffer.size() > current_offset) {
        LOG_WARN() << "attempted to read data from the out-of-bounds offset="
                   << pointer << ", with current_offset=" << current_offset;
        throw std::out_of_range("pointer is out of range");
    }

    std::memcpy(buffer.data(), m_data.data() + pointer, buffer.size());
    return buffer.size();
}

address mock_data_store::link(const address& addr) {
    address new_fragments;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_refcounter.contains(frag)) {
                new_fragments.push(frag);
            }
            m_refcounter[frag]++;
        }
    }

    return new_fragments;
}

std::vector<std::size_t> mock_data_store::unlink(const address& addr) {
    std::unordered_set<std::size_t> pages_to_free;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_refcounter.find(frag);
            if (it == m_refcounter.end()) {
                throw std::exception();
            }
            if (--(it->second) == 0) {
                std::fill(m_data.begin() + frag.pointer,
                          m_data.begin() + frag.pointer + frag.size, 0);
                m_refcounter.erase(it);
                pages_to_free.insert(frag.pointer / m_conf.page_size);
            }
        }
    }

    std::vector<std::size_t> sorted_pages_to_free(pages_to_free.begin(),
                                                  pages_to_free.end());
    std::sort(sorted_pages_to_free.begin(), sorted_pages_to_free.end());
    return sorted_pages_to_free;
}

uint64_t mock_data_store::get_used_space() const noexcept {
    return m_current_offset.load();
}

size_t mock_data_store::get_available_space() const noexcept {
    return m_conf.max_data_store_size - m_current_offset.load();
}

std::size_t mock_data_store::get_write_offset() const noexcept {
    return m_current_offset.load();
}

void mock_data_store::clear() {
    m_data.clear();
    m_refcounter.clear();
}

size_t mock_data_store::id() const noexcept { return m_data_store_id; }

allocation_t mock_data_store::allocate(std::size_t size,
                                       std::size_t alignment) {

    std::unique_lock lock(m_mutex);

    if (m_conf.max_data_store_size - m_current_offset.load() < size) {
        throw std::runtime_error("datastore cannot store additional " +
                                 std::to_string(size) + " bytes");
    }

    if (m_current_offset % alignment != 0) {
        m_current_offset += alignment - (m_current_offset % alignment);
    }
    std::size_t allocation_offset = m_current_offset.load();
    m_current_offset += size;

    return {.offset = allocation_offset, .size = size};
}

std::size_t mock_data_store::get_page_size() const noexcept {
    return m_conf.page_size;
}

mock_data_store::~mock_data_store() {
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
