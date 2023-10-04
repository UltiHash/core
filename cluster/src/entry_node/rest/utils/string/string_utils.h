#pragma once

#include <string>

namespace uh::cluster::rest::utils
{

    class string_utils
    {
    public:
        static std::string to_lower(const char* source);
    };

} // uh::cluster::rest::utils
