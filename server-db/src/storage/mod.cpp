#include "mod.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <util/exception.h>
#include "storage_backend.h"
#include <storage/backends/dump_storage.h>


namespace uh::dbn::storage
{

namespace
{

// ---------------------------------------------------------------------

void maybe_create_database_root_directory(std::filesystem::path db_root,
                                             bool create_new_root){
    if(!(std::filesystem::exists(db_root))) {
        // The root path does not exist. Should we create a new one?
        if(!create_new_root) {
            // We don't want to create a new root for the db;
            // throw an error if the root requested does not exist
            std::string msg("Path does not exist: " + db_root.string());
            throw std::runtime_error(msg);
        }else{
            // We want to create a new root.
            // If the root fails to be created, throw an error.
            if(!std::filesystem::create_directories(db_root)){
                std::string msg("Unable to create path for database root: " + db_root.string());
                throw std::runtime_error(msg);
            }
            else{
                INFO << "Created new database root at " << db_root;
            }
        }
    }
    else{
        //The root path exists, inform about it:
        INFO << "Found existing database root at " << db_root;
    }
}

BackendTypeEnum define_storage_backend_type(std::string backend_type){
    auto it = available_backend_types.find(backend_type);
    if (it != available_backend_types.end()) {
        return it->second;
    } else {
        std::string msg("Not a storage backend type: " + backend_type);
        THROW(util::exception, msg); 
    }
}

std::unique_ptr<storage_backend> make_storage_backend(const storage_config& cfg){

    maybe_create_database_root_directory(cfg.db_root, cfg.create_new_root);
    auto backend_type = define_storage_backend_type(cfg.backend_type);

    switch (backend_type)
    {
        case BackendTypeEnum::DumpStorage:
            return std::make_unique<storage::dump_storage>(cfg.db_root, cfg.size_bytes);
        case BackendTypeEnum::OtherStorage:
            THROW(util::exception, "Not implemented yet");  // TODO
    }
    
    std::string msg("Not a storage backend type: " + cfg.backend_type);
    THROW(util::exception, msg); 

}
    

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const storage_config& cfg);
    std::unique_ptr<storage_backend> some_storage_backend;
};

// ---------------------------------------------------------------------

//TODO
mod::impl::impl(const storage_config& cfg)
    : some_storage_backend(make_storage_backend(cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const storage_config& cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting storage module";
    //m_impl->some_storage_backend->start();
}

// ---------------------------------------------------------------------

//TODO
size_t mod::free_space()
{
    return 1;
}

// ---------------------------------------------------------------------

//TODO
uh::protocol::blob mod::read_chunk(const uh::protocol::blob& hash)
{
    return m_impl->some_storage_backend->read_chunk(hash);
}

// ---------------------------------------------------------------------

//TODO
uh::protocol::blob mod::write_chunk(const uh::protocol::blob& hash)
{
    return m_impl->some_storage_backend->write_chunk(hash);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
