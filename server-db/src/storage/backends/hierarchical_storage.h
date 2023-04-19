//
// Created by masi on 4/18/23.
//

#ifndef CORE_HIERARCHICAL_STORAGE_H
#define CORE_HIERARCHICAL_STORAGE_H
#include "storage/backend.h"
#include <metrics/storage_metrics.h>


namespace uh::dbn::storage {

class hierarchical_storage : public backend {

public:

    hierarchical_storage (std::filesystem::path db_root, size_t size_bytes, uh::dbn::metrics::storage_metrics& storage_metrics, std::size_t hash_directory_names_offset = 0);

    void start() override;

    std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

    std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size) override;

private:

    class hierarchical_allocation;

    void update_space_consumption();

    [[nodiscard]] std::filesystem::path get_hash_path (const std::string &hash) const;

    constexpr static std::string_view m_type = "HierarchicalStorage";
    constexpr static unsigned int m_levels = 4;

    const std::filesystem::path m_root;
    const std::size_t m_hash_directory_names_offset;

    const std::size_t m_alloc;
    std::atomic<std::size_t> m_used;
    uh::dbn::metrics::storage_metrics& m_storage_metrics;
};

}
#endif //CORE_HIERARCHICAL_STORAGE_H
