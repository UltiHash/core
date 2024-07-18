#ifndef UH_CLUSTER_REFERENCE_COUNTER_H
#define UH_CLUSTER_REFERENCE_COUNTER_H

#include <cstdint>
#include <filesystem>
#include <lmdb++.h>
#include <map>
#include <set>

namespace uh::cluster {
class reference_counter {
public:
    explicit reference_counter(const std::filesystem::path& root);
    std::map<std::size_t, std::size_t> decrement(const std::set<std::size_t>& pages);
    std::map<std::size_t, std::size_t> increment(const std::set<std::size_t>& pages);
private:
    lmdb::env m_env;

};
}
#endif // UH_CLUSTER_REFERENCE_COUNTER_H
