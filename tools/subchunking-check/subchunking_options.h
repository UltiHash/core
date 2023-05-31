//
// Created by masi on 4/26/23.
//

#ifndef CORE_SUBCHUNKING_OPTIONS_H
#define CORE_SUBCHUNKING_OPTIONS_H
#include <options/options.h>
#include <string>

namespace uh::tools {

struct subchunking_config {
    std::string path;
};

class subchunking_options : public uh::options::options {
public:
    subchunking_options();

    uh::options::action evaluate(const boost::program_options::variables_map &vars) override;

    [[nodiscard]] const subchunking_config &config() const;

private:
    subchunking_config m_config;
};

}
#endif //CORE_SUBCHUNKING_OPTIONS_H
