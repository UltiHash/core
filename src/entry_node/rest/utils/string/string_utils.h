#pragma once

#include <string>

namespace uh::cluster::rest::utils
{

    class string_utils
    {
    public:
        static std::string to_lower(const char* source);
        static bool is_same(const char* string1, const char* string2);
        static bool is_bool(const std::string&);
        static std::string URL_encode(const std::string&);
    };

} // uh::cluster::rest::utils
