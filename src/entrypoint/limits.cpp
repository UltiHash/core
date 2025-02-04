#include "limits.h"

#include <common/telemetry/log.h>
#include <entrypoint/http/command_exception.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster {

limits::limits(license_watcher& watcher)
    : m_watcher{watcher},
      m_data_storage_size{0ull} {}

void limits::set_storage_size(std::size_t size) {
    m_data_storage_size.store(size);
}

void limits::check_storage_size(std::size_t increment) {
    auto new_size = m_data_storage_size.load() + increment;
    auto max_data_size = m_watcher.get_license().storage_cap_gib * GIBI_BYTE;
    if (new_size > max_data_size) {
        throw command_exception(status::insufficient_storage,
                                "StorageLimitExceeded", "insufficient storage");
    }

    if (new_size * 100 > max_data_size * SIZE_LIMIT_WARNING_PERCENTAGE) {
        if (m_warn_counter == 0) {
            LOG_WARN() << "over " << SIZE_LIMIT_WARNING_PERCENTAGE
                       << "% of storage limit reached";
            m_warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
        }

        --m_warn_counter;
    }
}

} // namespace uh::cluster
