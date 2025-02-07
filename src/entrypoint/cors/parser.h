#pragma once

#include "info.h"

namespace uh::cluster::ep::cors {

class parser {
public:
    static std::map<std::string, info> parse(std::string code);
    static std::map<std::string, info> parse_json(std::string code);
};

} // namespace uh::cluster::ep::cors
