#include "mod.h"
#include "storage/backends/hierarchical_storage.h"
#include "storage/backends/smart_storage.h"

#include <unistd.h> //rmdir
#include <logging/logging_boost.h>
#include <util/exception.h>
#include <storage/options.h>

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

void maybe_create_database_root_directory(std::filesystem::path db_root,
                                             bool ok_create_new_root){


    //Check whether the directory already exists:
    bool no_db_root = !std::filesystem::is_directory(db_root);

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

BackendTypeEnum define_backend_type(const std::string& backend_type){
    auto it = string2backendtype.find(backend_type);
    if (it != string2backendtype.end()) {
        return it->second;
    } else {
        std::string msg("Not a storage backend type: " + backend_type);
        THROW(util::exception, msg);
    }
}

// ---------------------------------------------------------------------

std::unique_ptr<backend> make_backend(const storage_config& cfg, metrics::storage_metrics& storage_metrics,
                                      state::scheduled_compressions_state& scheduled_compressions)
{

    maybe_create_database_root_directory(cfg.db_root, cfg.create_new_directory);

    //Check whether we have enough space:
    size_t max_size = max_configurable_capacity(cfg.db_root);
    size_t size_needed = cfg.allocate_bytes > 0 ? cfg.allocate_bytes : max_size;
    if(size_needed > max_size){
        THROW(util::exception, "Requesting to allocate more space than available. Unable to make a storage backend");
    }

    auto backend_type = define_backend_type(cfg.backend_type);

    switch (backend_type)
    {
        case BackendTypeEnum::HierarchicalStorage:
            return std::make_unique<storage::hierarchical_storage>(
                    hierarchical_storage_config{ cfg.db_root, size_needed, cfg.comp },
                    storage_metrics, scheduled_compressions);
        case BackendTypeEnum::SmartStorage:
            return std::make_unique<storage::smart_storage> (
                    smart::make_smart_config(cfg.db_root, size_needed, cfg.max_file_size),
                    storage_metrics);
        case BackendTypeEnum::OtherStorage:
            THROW(util::exception, "Not yet implemented backend type");
    }

    std::string msg("Not a storage backend type: " + cfg.backend_type);
    THROW(util::exception, msg);

}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const storage_config& cfg, metrics::storage_metrics& storage_metrics, state::scheduled_compressions_state& scheduled_compressions);
    std::unique_ptr<storage::backend> m_backend;
};

// ---------------------------------------------------------------------

mod::impl::impl(const storage_config& cfg, metrics::storage_metrics& storage_metrics, state::scheduled_compressions_state& scheduled_compressions)
    : m_backend(make_backend(cfg, storage_metrics, scheduled_compressions))
{
}

// ---------------------------------------------------------------------

mod::mod(const storage_config& cfg, metrics::storage_metrics& storage_metrics, state::scheduled_compressions_state& scheduled_compressions)
    : m_impl(std::make_unique<impl>(cfg, storage_metrics, scheduled_compressions))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start() const
{
    INFO << "starting storage module";
    m_impl->m_backend->start();
}

// ---------------------------------------------------------------------

void mod::stop() const
{
    INFO << "stopping storage module";
    m_impl->m_backend->stop();
}

// ---------------------------------------------------------------------

storage::backend& mod::backend()
{
    return *m_impl->m_backend;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
