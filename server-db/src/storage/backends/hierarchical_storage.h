//
// Created by masi on 4/18/23.
//

#ifndef CORE_HIERARCHICAL_STORAGE_H
#define CORE_HIERARCHICAL_STORAGE_H

#include <storage/backend.h>
#include <storage/compressed_file_store.h>
#include <state/scheduled_compressions_state.h>
#include "io/sha512.h"
#include "storage/backends/smart_backend/smart_config.h"
#include <metrics/storage_metrics.h>
#include "logging/logging_boost.h"
#include "util/exception.h"

namespace uh::dbn::storage
{

struct hierarchical_storage_config
{
    /**
     * Root directory of storage
     */
    std::filesystem::path db_root;

    /**
     * Amount of allocated space in bytes.
     */
    size_t size_bytes;

    /**
     * Configuration of compressed file storage.
     */
    compressed_file_store_config compressed;

    /**
     * Configuration of smart storage.
     */
    smart::smart_config smart_post_processing;
};

class hierarchical_storage: public backend
{

public:
    hierarchical_storage(const hierarchical_storage_config &config,
                         uh::dbn::metrics::storage_metrics &storage_metrics,
                         state::scheduled_compressions_state &scheduled_compressions);

    void start() override;
    void stop() override;

    std::unique_ptr<io::data_generator> read_block(const std::span<char> &hash) override;

    size_t free_space() override;

    size_t used_space() override;

    size_t allocated_space() override;

    std::string backend_type() override;

    std::pair <std::size_t, std::vector <char>> write_block (const std::span <const char>& data) override;

    [[nodiscard]] std::filesystem::path get_hash_path(const std::string_view &hash) const;

    static constexpr std::size_t BUFFER_SIZE = 128 * 1024;
private:
    void update_space_consumption();
    void return_space(std::size_t size);

    void acquire_storage_size(std::size_t size);

    constexpr static std::string_view m_type = "HierarchicalStorage";
    constexpr static unsigned int m_levels = 4;

    const std::filesystem::path m_root;

    const std::size_t m_alloc;
    std::atomic<std::size_t> m_used;
    uh::dbn::metrics::storage_metrics &m_storage_metrics;
    compressed_file_store m_store;
};

}
#endif //CORE_HIERARCHICAL_STORAGE_H
