#ifndef SERVER_DATABASE_STORAGE_MOD_H
#define SERVER_DATABASE_STORAGE_MOD_H

#include <protocol/client_pool.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include "utils.h"
#include "storage_backend.h"


namespace uh::dbn::storage
{

enum class BackendTypeEnum {DumpStorage, OtherStorage};

static std::unordered_map<std::string, BackendTypeEnum> const available_backend_types = {
    {"DumpStorage", BackendTypeEnum::DumpStorage },
    {"OtherStorage", BackendTypeEnum::OtherStorage}
};

// ---------------------------------------------------------------------

struct storage_config
{
    constexpr static std::string_view default_db_root = "./default/db/root";
    constexpr static std::string_view default_backend_type = "DumpStorage";
    std::size_t size_bytes = 1e9;
    std::filesystem::path db_root = std::string(default_db_root);
    std::string backend_type = std::string(default_backend_type);
    bool create_new_root = false;
};

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const storage_config& cfg);
    ~mod();

    void start();

    size_t free_space();

    uh::protocol::blob read_chunk(const uh::protocol::blob& hash);

    uh::protocol::blob write_chunk(const uh::protocol::blob& hash);

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
