#pragma once

#include <string>
#include <vector>

namespace uh::cluster {

class string_utils {
  public:
    static std::string URL_encode(const std::string&);
    static std::vector<std::string>::const_iterator
    find_lexically_closest(const std::vector<std::string>& strings,
                           const std::string& compareTo);
};

} // namespace uh::cluster
