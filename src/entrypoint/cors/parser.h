#pragma once

#include "info.h"
#include <vector>

namespace uh::cluster::ep::cors {

class parser {
public:
    static std::vector<info> parse(std::string code);
};

} // namespace uh::cluster::ep::cors
