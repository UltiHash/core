#include "options.h"


using namespace boost::program_options;

namespace uh::dbn::storage
{

enum class OptionsEnum {DbRoot, CreateNewRoot, DbStorageAlgorithm, AllocateStorage};

constexpr const char* optionString(OptionsEnum n)
{
    switch (n)
    {
        case OptionsEnum::DbRoot: return "db-root";
        case OptionsEnum::CreateNewRoot: return "create-new-root";
        case OptionsEnum::DbStorageAlgorithm: return "db-storage-algorithm";
        case OptionsEnum::AllocateStorage: return "allocate-storage";
        default: THROW(util::exception, "Not implemented option");
    }
}

// ---------------------------------------------------------------------

options::options()
    : uh::options::options("Storage Options")
{
    visible().add_options()
        (optionString(OptionsEnum::DbRoot),
            value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_db_root)),
                "Directory where the database root is located")
        (optionString(OptionsEnum::CreateNewRoot),
            "Creates the directory defined by argument db-root if it does not exists.")
        (optionString(OptionsEnum::DbStorageAlgorithm),
            value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_backend_type)),
                "Database chunk sorting algorithm. One of [DumpStorage | OtherStorage]")
        (optionString(OptionsEnum::AllocateStorage),
            value<size_t>()->default_value(uh::dbn::storage::storage_config::default_allocated_size),
                "Space in bytes to allocate to this storage backend. Zero for maximum");
}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    storage_config c;
    c.db_root = vars[optionString(OptionsEnum::DbRoot)].as<std::string>();
    c.backend_type = vars[optionString(OptionsEnum::DbStorageAlgorithm)].as<std::string>();
    c.allocate_bytes = vars[optionString(OptionsEnum::AllocateStorage)].as<std::size_t>();

    if (std::filesystem::path (c.db_root).is_relative()) {
        THROW(util::illegal_args, "The database root must be an absolute path.");
    }

    if (vars.count(optionString(OptionsEnum::CreateNewRoot)) > 0)
    {
        c.create_new_root = true;
    }

    std::swap(m_config, c);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const storage_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
