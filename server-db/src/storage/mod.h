#ifndef SERVER_DATABASE_STORAGE_MOD_H
#define SERVER_DATABASE_STORAGE_MOD_H

#include <protocol/client_pool.h>
#include <metrics/storage_metrics.h>
#include <persistence/scheduled_compressions_persistence.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include "utils.h"
#include <storage/backend.h>
#include <storage/compressed_file_store.h>
#include <storage/storage_config.h>


namespace uh::dbn::storage
{

enum class BackendTypeEnum {DumpStorage, HierarchicalStorage, SmartStorage, OtherStorage};

static std::unordered_map<std::string, BackendTypeEnum> string2backendtype = {
  {"HierarchicalStorage", BackendTypeEnum::HierarchicalStorage},
  {"SmartStorage", BackendTypeEnum::SmartStorage},
  {"OtherStorage", BackendTypeEnum::OtherStorage}
};

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const storage_config& cfg, metrics::storage_metrics& storage_metrics,
        persistence::scheduled_compressions_persistence& scheduled_compressions);
    ~mod();

    void start() const;
    void stop() const;

    storage::backend& backend();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
