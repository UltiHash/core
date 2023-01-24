#include "options.h"


using namespace boost::program_options;

namespace uh::dbn::storage
{

//TODO - I'd like to use the following enum to define options,
// in order to avoid hard-coding them many times around.
enum class OptionsEnum {DbRoot, CreateNewRoot, DbStorageAlgorithm};
static std::unordered_map<OptionsEnum, std::string> const options_str = {
    {OptionsEnum::DbRoot, "db-root" },
    {OptionsEnum::CreateNewRoot, "create-new-root"},
    {OptionsEnum::DbStorageAlgorithm, "db-storage-algorithm" }
};

// ---------------------------------------------------------------------

options::options()
    : m_desc("Storage Options")
{
    m_desc.add_options()
        ("db-root", value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_db_root)), "Directory where the database root is located")
        ("create-new-root", "Creates the directory defined by argument db-root if it does not exists.")
        ("db-storage-algorithm", value<std::string>()->default_value(std::string(uh::dbn::storage::storage_config::default_backend_type)), "Database chunk sorting algorithm. One of [DumpStorage | OtherStorage]");
}

// ---------------------------------------------------------------------

void options::apply(uh::options::options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    storage_config c;
    c.db_root = vars["db-root"].as<std::string>();
    c.backend_type = vars["db-storage-algorithm"].as<std::string>();

    if (vars.count("create-new-root") > 0)
    {
        c.create_new_root = true;
    }

    std::swap(m_config, c);
}

// ---------------------------------------------------------------------

const storage_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
