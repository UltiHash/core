#ifndef SERVER_DATABASE_STORAGE_MOD_H
#define SERVER_DATABASE_STORAGE_MOD_H

#include <protocol/client_pool.h>
#include <metrics/storage_metrics.h>
#include <persistence/storage/scheduled_compressions_persistence.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include "utils.h"
#include <storage/backend.h>
#include <storage/compressed_file_store.h>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

enum class BackendTypeEnum {DumpStorage, HierarchicalStorage, OtherStorage};

static std::unordered_map<std::string, BackendTypeEnum> string2backendtype = {
  {"DumpStorage", BackendTypeEnum::DumpStorage},
  {"HierarchicalStorage", BackendTypeEnum::HierarchicalStorage},
  {"OtherStorage", BackendTypeEnum::OtherStorage}
};

// ---------------------------------------------------------------------

struct storage_config
{
    constexpr static std::string_view default_db_root = "./DEFAULT_DB_ROOT";
    constexpr static std::string_view default_backend_type = "HierarchicalStorage";
    constexpr static size_t default_allocated_size = 0;
    size_t allocate_bytes = 0;
    std::filesystem::path db_root = std::string(default_db_root);
    std::string backend_type = std::string(default_backend_type);
    bool create_new_root = false;

    compressed_file_store_config comp;
};

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const storage_config& cfg, metrics::storage_metrics& storage_metrics, persistence::scheduled_compressions_persistence& scheduled_compressions);
    ~mod();

    void start();

    storage::backend& backend();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
