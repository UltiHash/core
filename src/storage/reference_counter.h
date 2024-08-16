#ifndef UH_CLUSTER_REFERENCE_COUNTER_H
#define UH_CLUSTER_REFERENCE_COUNTER_H

#include <filesystem>

#include <functional>
#include <lmdbxx/lmdb++.h>

namespace uh::cluster {
class reference_counter {
public:
    reference_counter(
        const std::filesystem::path& root, std::size_t page_size,
        const std::function<void(std::size_t offset, std::size_t size)>& cb);
    void decrement(std::size_t offset, std::size_t size);
    bool increment(std::size_t offset, std::size_t size, bool init = false);

private:
    lmdb::env m_env;
    std::size_t m_page_size;
    std::function<void(std::size_t offset, std::size_t size)> m_cb;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_REFERENCE_COUNTER_H
