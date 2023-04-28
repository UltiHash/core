//
// Created by masi on 4/26/23.
//
#include "dedup_options.h"
#include <filesystem>

uh::tools::dedup_options::dedup_options(): uh::options::options("Chunking Options") {
    visible().add_options()
            ("path", boost::program_options::value< std::string > (&m_config.path), "directory to be deduplicated");
}

uh::options::action uh::tools::dedup_options::evaluate(const boost::program_options::variables_map &vars) {
    auto c = m_config;
    c.path = vars["path"].as<std::string>();

    if (!std::filesystem::exists(c.path)) {
        throw std::exception ();
    }
    std::swap(m_config, c);
    return uh::options::action::proceed;
}

const uh::tools::dedup_config &uh::tools::dedup_options::config() const {
    return m_config;
}
