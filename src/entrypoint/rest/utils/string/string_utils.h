#pragma once

#include <string>
#include <vector>

namespace uh::cluster::rest::utils {

class string_utils {
public:
    static std::string to_lower(const char* source);
    static bool is_same(const char* string1, const char* string2);
    static bool is_bool(const std::string&);
    static std::string URL_encode(const std::string&);
    static std::vector<std::string>::iterator
    find_lexically_closest(std::vector<std::string>&, const std::string&);
};

} // namespace uh::cluster::rest::utils
