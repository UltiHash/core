#ifndef UH_CLUSTER_REFERENCE_COUNTER_H
#define UH_CLUSTER_REFERENCE_COUNTER_H

#include <filesystem>

#include <lmdbxx/lmdb++.h>

namespace uh::cluster {
class reference_counter {
public:
    explicit reference_counter(const std::filesystem::path& root,
                               std::size_t page_size);
    void decrement(std::size_t offset, std::size_t size);
    void increment(std::size_t offset, std::size_t size);

private:
    lmdb::env m_env;
    std::size_t m_page_size;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_REFERENCE_COUNTER_H
