#pragma once

#include <common/license/license_watcher.h>

#include <atomic>

namespace uh::cluster {

class limits {
public:
    limits(license_watcher& watcher);

    /**
     * Set storage size without checking.
     */
    void set_storage_size(std::size_t size);

    /**
     * Check internal storage size and increment the counter.
     */
    void check_storage_size(std::size_t increment);

    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

private:
    license_watcher& m_watcher;
    std::atomic<std::size_t> m_data_storage_size;
    unsigned m_warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
};

} // namespace uh::cluster
