//
// Created by masi on 4/26/23.
//
#include "subchunking_options.h"
#include <filesystem>
#include "util/exception.h"

uh::tools::subchunking_options::subchunking_options(): uh::options::options("Subchunking Options") {
    visible().add_options()
            ("path", boost::program_options::value< std::string > (&m_config.path), "directory to be deduplicated");
}

uh::options::action uh::tools::subchunking_options::evaluate(const boost::program_options::variables_map &vars) {
    auto c = m_config;
    c.path = vars["path"].as<std::string>();

    if (!std::filesystem::exists(c.path)) {
        THROW(util::exception, "The path does not exist!");
    }
    std::swap(m_config, c);
    return uh::options::action::proceed;
}

const uh::tools::subchunking_config &uh::tools::subchunking_options::config() const {
    return m_config;
}
