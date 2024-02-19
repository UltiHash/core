#include "string_utils.h"
#include <boost/url.hpp>
#include <boost/url/encode.hpp>

namespace uh::cluster {

constexpr boost::urls::grammar::lut_chars custom_unreserved_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-._~/";

std::string
string_utils::URL_encode(const std::string& str_to_encode) noexcept {
    auto encoded_string =
        boost::urls::encode(str_to_encode, custom_unreserved_chars);

    return encoded_string;
}

std::vector<std::string>::const_iterator
string_utils::find_lexically_closest(const std::vector<std::string>& strings,
                                     const std::string& compareTo) {
    if (strings.empty()) {
        return strings.end();
    }

    if (compareTo.empty()) {
        return strings.begin();
    }

    auto lexicalCompare = [](const std::string& a, const std::string& b) {
        return a < b;
    };

    auto nextDifferentItr = std::lower_bound(strings.begin(), strings.end(),
                                             compareTo, lexicalCompare);

    if (nextDifferentItr != strings.end() && *nextDifferentItr == compareTo) {
        ++nextDifferentItr;
    }

    return nextDifferentItr;
}

} // namespace uh::cluster
