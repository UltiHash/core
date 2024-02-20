#pragma once

#include <string>
#include <vector>

namespace uh::cluster {

class string_utils {
public:
    static std::string url_encode(const std::string&) noexcept;
    static std::vector<std::string>::const_iterator
    find_lexically_closest(const std::vector<std::string>& strings,
                           const std::string& compareTo);
    static bool is_bool(const std::string& str_to_eval);
};

} // namespace uh::cluster
