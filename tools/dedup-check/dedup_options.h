//
// Created by masi on 4/26/23.
//

#ifndef CORE_DEDUP_OPTIONS_H
#define CORE_DEDUP_OPTIONS_H
#include <options/options.h>
#include <string>

namespace uh::tools {

struct dedup_config {
    std::string path;
};

class dedup_options : public uh::options::options {
public:
    dedup_options();

    uh::options::action evaluate(const boost::program_options::variables_map &vars) override;

    const dedup_config &config() const;

private:
    dedup_config m_config;
};

}
#endif //CORE_DEDUP_OPTIONS_H
