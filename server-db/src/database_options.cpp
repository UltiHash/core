#include "database_options.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

database_options::database_options()
    : m_desc("Database Options")
{
    m_desc.add_options()
        ("db-root", value<std::string>()->default_value(std::string(uh::dbn::db_config::DEFAULT_DB_ROOT)), "Directory where the database root is located")
        ("create-new-root", "Creates the directory defined by argument db-root if it does not exists.");
}

// ---------------------------------------------------------------------

void database_options::apply(options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void database_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_config.db_root = vars["db-root"].as<std::string>();

    if (vars.count("create-new-root") > 0)
    {
        m_config.create_new_root = true;
    }

}

// ---------------------------------------------------------------------

const dbn::db_config& database_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::options
