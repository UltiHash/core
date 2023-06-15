#ifndef SERVER_DATABASE_STORAGE_STORAGE_CONFIG_H
#define SERVER_DATABASE_STORAGE_STORAGE_CONFIG_H

#include <compression/type.h>

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

struct compressed_file_store_config
{
    static constexpr unsigned DEFAULT_THREADS = 5u;

    /**
     * Number of threads for background compression.
     */
    unsigned threads = DEFAULT_THREADS;

    /**
     * Default compression applied to files.
     */
    comp::type compression = comp::type::none;
};

// ---------------------------------------------------------------------

struct storage_config
{
    constexpr static std::string_view default_data_directory = "/var/lib/uh-data-node";
    constexpr static std::string_view default_backend_type = "HierarchicalStorage";
    constexpr static size_t default_allocated_size = 0;
    size_t allocate_bytes = 0;
    size_t max_file_size = 1024ul * 1024ul * 1024ul * 512ul;

    std::filesystem::path data_directory = default_data_directory;
    std::filesystem::path db_root = std::string(default_data_directory) + std::string("/data");
    std::filesystem::path db_metrics = std::string(default_data_directory) + std::string("/state");
    std::string backend_type = std::string(default_backend_type);
    bool create_new_directory = false;

    compressed_file_store_config comp;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
