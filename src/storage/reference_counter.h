#ifndef UH_CLUSTER_REFERENCE_COUNTER_H
#define UH_CLUSTER_REFERENCE_COUNTER_H

#include <cstdint>
#include <filesystem>
#include <lmdb++.h>
#include <vector>

namespace uh::cluster {
class reference_counter {
public:
    explicit reference_counter(const std::filesystem::path& root,
                               const std::size_t page_size);
    void decrement(const std::size_t offset, const std::size_t size);
    void increment(const std::size_t offset, const std::size_t size);

private:
    lmdb::env m_env;
    std::size_t m_page_size;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_REFERENCE_COUNTER_H
