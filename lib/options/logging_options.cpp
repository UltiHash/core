#include "logging_options.h"

#include <ranges>


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

logging_options::logging_options()
    : m_desc("Logging Options")
{
    m_desc.add_options()
        ("log-stdout,O", value< std::string >(), "write log messages to stdout, specify log level")
        ("log-stderr,E", value< std::string >()->implicit_value("INFO"), "write log messages to stderr, specify log level")
        ("log-file,F", value< std::vector<std::string> >(), "write log messages to file, specify path and level");
}

// ---------------------------------------------------------------------

void logging_options::apply(options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void logging_options::evaluate(const boost::program_options::variables_map& vars)
{
    log::config config;

    {
        auto entry = vars["log-stdout"];
        if (!entry.empty())
        {
            log::sink_config cfg = { .type = uh::log::sink_type::cout };

            if (!entry.as<std::string>().empty())
            {
                cfg.level = uh::log::severity_from_string(entry.as<std::string>());
            }

            config.sinks.push_back(cfg);
        }
    }

    {
        auto entry = vars["log-stderr"];
        if (!entry.empty())
        {
            log::sink_config cfg = { .type = uh::log::sink_type::cerr };

            if (!entry.as<std::string>().empty())
            {
                cfg.level = uh::log::severity_from_string(entry.as<std::string>());
            }

            config.sinks.push_back(cfg);
        }
    }

    if (vars.count("log-file") > 0)
    {
        for (const auto& out_opt : vars["log-file"].as< std::vector<std::string> >())
        {
            log::sink_config cfg = { .type = uh::log::sink_type::file };

            if (out_opt.empty())
            {
                throw std::runtime_error("log file path required");
            }

            auto colon = out_opt.find(':');
            if (colon != std::string::npos)
            {
                cfg.level = uh::log::severity_from_string(out_opt.substr(colon + 1));
                cfg.filename = out_opt.substr(0, colon);
            }
            else
            {
                cfg.filename = out_opt;
            }

            config.sinks.push_back(cfg);
        }
    }

    std::swap(config, m_config);
}

// ---------------------------------------------------------------------

const log::config& logging_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

}
