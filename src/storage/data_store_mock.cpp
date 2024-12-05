
#include "data_store_mock.h"
#include "common/telemetry/log.h"
#include "common/telemetry/metrics.h"
#include "common/utils/pointer_traits.h"
#include <algorithm>
#include <mutex>

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
      m_conf{conf} {

    m_data.reserve(m_conf.max_data_store_size);
}

address data_store_mock::write(const std::string_view& data) {
    if (m_data.size() + data.size() > m_conf.max_data_store_size or
        data.size() > static_cast<size_t>(m_conf.max_file_size)) [[unlikely]] {
        throw std::bad_alloc();
    }

    m_data.insert(m_data.end(), data.cbegin(), data.cend());

    address data_address;
    data_address.push({.pointer = pointer_traits::get_global_pointer(
                           m_data.size(), m_storage_id, m_data_store_id),
                       .size = data.size()});
    return data_address;
}

void data_store_mock::manual_write(uint64_t internal_pointer,
                                   const std::string_view& data) {

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

address data_store_mock::link(const address& addr) { return {}; }

size_t data_store_mock::unlink(const address& addr) { return 0; }

uint64_t data_store_mock::get_used_space() const noexcept {
    return m_data.size();
}

size_t data_store_mock::get_available_space() const noexcept {
    return m_conf.max_data_store_size - m_data.size();
}

size_t data_store_mock::id() const noexcept { return m_data_store_id; }

data_store_mock::~data_store_mock() {}

} // end namespace uh::cluster
