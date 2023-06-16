#include "options.h"


using namespace boost::program_options;

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

options::options()
    : uh::options::options("Storage Options")
{
    visible().add_options()
        (optionString(OptionsEnum::DataDirectory),
            value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_data_directory)),
                "Directory where the data node stores its data")
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
        std::filesystem::create_directory(c.data_directory);
    }

    c.db_root = c.data_directory / std::filesystem::path("data");
    c.db_metrics = c.data_directory / std::filesystem::path("state");

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
