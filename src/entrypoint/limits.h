#ifndef CORE_ENTRYPOINT_LIMITS_H
#define CORE_ENTRYPOINT_LIMITS_H

#include <atomic>

namespace uh::cluster {

class limits {
public:
    limits(std::size_t max_data_size);

    /**
     * Set storage size without checking.
     */
    void storage_size(std::size_t size);

    /**
     * Check internal storage size and increment the counter.
     */
    void check_storage_size(std::size_t increment);

    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

private:
    std::size_t m_max_data_size;
    std::atomic<std::size_t> m_data_storage_size;
    unsigned m_warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
};

} // namespace uh::cluster

#endif
