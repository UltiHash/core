#ifndef SERVER_DATABASE_STORAGE_MOD_H
#define SERVER_DATABASE_STORAGE_MOD_H

#include <protocol/client_pool.h>
#include <metrics/storage_metrics.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include "utils.h"
#include "storage_backend.h"


namespace uh::dbn::storage
{

enum class BackendTypeEnum {DumpStorage, OtherStorage};

static std::unordered_map<std::string, BackendTypeEnum> string2backendtype = {
  {"DumpStorage", BackendTypeEnum::DumpStorage},
  {"OtherStorage", BackendTypeEnum::OtherStorage}
};

// ---------------------------------------------------------------------

struct storage_config
{
    constexpr static std::string_view default_db_root = "./DEFAULT_DB_ROOT";
    constexpr static std::string_view default_backend_type = "DumpStorage";
    constexpr static size_t default_allocated_size = 0;
    size_t allocate_bytes = 0;
    std::filesystem::path db_root = std::string(default_db_root);
    std::string backend_type = std::string(default_backend_type);
    bool create_new_root = false;
};

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const storage_config& cfg, metrics::storage_metrics& storage_metrics);
    ~mod();

    void start();

    size_t free_space();

    std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash);

    uh::protocol::block_meta_data write_block(const uh::protocol::blob& hash);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
