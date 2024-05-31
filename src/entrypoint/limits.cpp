#include "limits.h"

#include "common/telemetry/log.h"
#include "common/utils/error.h"

namespace uh::cluster {

limits::limits(std::size_t max_data_size)
    : m_max_data_size(max_data_size),
      m_data_storage_size(0ull) {}

void limits::storage_size(std::size_t size) { m_data_storage_size = size; }

void limits::check_storage_size(std::size_t increment) {
    auto new_size = m_data_storage_size + increment;
    if (new_size > m_max_data_size) {
        throw error_exception(error::storage_limit_exceeded);
    }

    if (new_size * 100 > m_max_data_size * SIZE_LIMIT_WARNING_PERCENTAGE) {
        if (m_warn_counter == 0) {
            LOG_WARN() << "over " << SIZE_LIMIT_WARNING_PERCENTAGE
                       << "% of storage limit reached";
            m_warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
        }

        --m_warn_counter;
    }

    m_data_storage_size = new_size;
}

void limits::free_storage_size(std::size_t decrement) {
    std::size_t current = m_data_storage_size;
    std::size_t desired;

    do {
        desired = current < decrement ? 0ull : current - decrement;
    } while (!m_data_storage_size.compare_exchange_weak(current, desired));
}

} // namespace uh::cluster
