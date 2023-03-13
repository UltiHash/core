#include "mod.h"

#include <config.hpp>

#include <unistd.h> //rmdir
#include <logging/logging_boost.h>
#include <util/exception.h>
#include <storage/backends/dump_storage.h>


namespace uh::dbn::storage
{

namespace
{

// ---------------------------------------------------------------------

void maybe_create_database_root_directory(std::filesystem::path db_root,
                                             bool ok_create_new_root){


    //Check whether the directory already exists:
    bool no_db_root = !std::filesystem::is_directory(db_root);

    //Check whether we want to create a new dir:
    if(no_db_root and not ok_create_new_root)
        THROW(util::exception, "Path does not exist: " + db_root.string());

    //We are OK creating a new root if needed, otherwise just inform about its existence
    if (no_db_root){
        if(!std::filesystem::create_directories(db_root)){
            std::string msg("Unable to create path for database root: " + db_root.string());
            THROW(util::exception, msg);
        }
        INFO << "Created new database root at " << db_root;
    }
    else{
        INFO << "Found existing database root at " << db_root;
    }
}

// ---------------------------------------------------------------------

BackendTypeEnum define_backend_type(std::string backend_type){
    auto it = string2backendtype.find(backend_type);
    if (it != string2backendtype.end()) {
        return it->second;
    } else {
        std::string msg("Not a storage backend type: " + backend_type);
        THROW(util::exception, msg);
    }
}

// ---------------------------------------------------------------------

std::unique_ptr<backend> make_backend(const storage_config& cfg, metrics::storage_metrics& storage_metrics){

    maybe_create_database_root_directory(cfg.db_root, cfg.create_new_root);

    //Check whether we have enough space:
    size_t max_size = max_configurable_capacity(cfg.db_root);
    size_t size_needed = cfg.allocate_bytes > 0 ? cfg.allocate_bytes : max_size;
    if(size_needed > max_size){
        THROW(util::exception, "Requesting to allocate more space than available. Unable to make a storage backend");
    }

    auto backend_type = define_backend_type(cfg.backend_type);

    switch (backend_type)
    {
        case BackendTypeEnum::DumpStorage:
            return std::make_unique<storage::dump_storage>(cfg.db_root, size_needed, storage_metrics);
        case BackendTypeEnum::OtherStorage:
            THROW(util::exception, "Not implemented yet");
    }

    std::string msg("Not a storage backend type: " + cfg.backend_type);
    THROW(util::exception, msg);

}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const storage_config& cfg, metrics::storage_metrics& storage_metrics);
    std::unique_ptr<storage::backend> m_backend;
};

// ---------------------------------------------------------------------

mod::impl::impl(const storage_config& cfg, metrics::storage_metrics& storage_metrics)
    : m_backend(make_backend(cfg, storage_metrics))
{
}

// ---------------------------------------------------------------------

mod::mod(const storage_config& cfg, metrics::storage_metrics& storage_metrics)
    : m_impl(std::make_unique<impl>(cfg, storage_metrics))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

storage::backend& mod::backend()
{
    return *m_impl->m_backend;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
