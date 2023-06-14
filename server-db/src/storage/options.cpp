#include "options.h"


using namespace boost::program_options;

namespace uh::dbn::storage
{

enum class OptionsEnum {DataDirectory, CreateNewRoot, DbStorageAlgorithm, AllocateStorage};

constexpr const char* optionString(OptionsEnum n)
{
    switch (n)
    {
        case OptionsEnum::DataDirectory: return "data-directory";
        case OptionsEnum::CreateNewRoot: return "create-new-directory";
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
        (optionString(OptionsEnum::DataDirectory),
            value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_data_directory)),
                "Directory where important data are persisted")
        (optionString(OptionsEnum::CreateNewRoot),
            "Creates the data directory if it does not exists.")
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
    c.data_directory = vars[optionString(OptionsEnum::DataDirectory)].as<std::string>();
    c.backend_type = vars[optionString(OptionsEnum::DbStorageAlgorithm)].as<std::string>();
    c.allocate_bytes = vars[optionString(OptionsEnum::AllocateStorage)].as<std::size_t>();

    if (vars.count(optionString(OptionsEnum::CreateNewRoot)) > 0)
    {
        c.create_new_directory = true;
    }

    if (std::filesystem::path(c.data_directory).is_relative())
    {
        THROW(util::illegal_args, "The data directory must be an absolute path.");
    }

    if (std::filesystem::exists(c.data_directory))
    {
        if (!std::filesystem::is_directory(c.data_directory))
        {
            THROW(util::illegal_args, "The given path for data directory is not a directory: " + c.data_directory.string());
        }
        if (access(c.data_directory.c_str(), W_OK | R_OK ) != 0)
        {
            THROW(util::illegal_args, "The user doesn't have read and write permission on directory: " + c.data_directory.string());
        }
    }
    else
    {
        if (c.create_new_directory)
        {
            std::filesystem::create_directory(c.data_directory);
        }
        else
        {
            THROW(util::illegal_args, "The data directory '" + std::string(c.data_directory) + "' doesn't exist. You can turn on the flag with the option '--" +
                                      std::string(optionString(OptionsEnum::CreateNewRoot)) + "' to create the "
                                                                                              "given directory in case it doesn't exist." );
        }
    }

    c.db_root = c.data_directory / std::filesystem::path("data");
    c.db_metrics = c.data_directory / std::filesystem::path("metrics");

    std::filesystem::create_directory(c.db_metrics);

    std::swap(m_config, c);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const storage_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

compression_options::compression_options()
    : uh::options::options("Compression Options")
{
    visible().add_options()

        ("comp-algorithm",
         value<std::string>()->default_value("none"),
         "compress chunks on disk using this algorithm")

        ("comp-threads",
         value<unsigned>(&m_config.threads)->default_value(compressed_file_store_config::DEFAULT_THREADS),
         "number of threads used for compression");
}

// ---------------------------------------------------------------------

uh::options::action compression_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_config.compression = comp::to_type(vars["comp-algorithm"].as<std::string>());
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const compressed_file_store_config& compression_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
