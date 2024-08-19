#include "strings.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/url.hpp>
#include <boost/url/encode.hpp>
#include <sstream>

using namespace boost;

namespace uh::cluster {

std::string_view trim(std::string_view in, std::string_view chars) {
    return ltrim(rtrim(in, chars), chars);
}

std::string_view ltrim(std::string_view in, std::string_view chars) {
    auto start = in.find_first_not_of(chars);
    if (start == std::string::npos) {
        return {};
    }

    return in.substr(start);
}

std::string_view rtrim(std::string_view in, std::string_view chars) {
    auto end = in.find_last_not_of(chars);
    if (end == std::string::npos) {
        return {};
    }

    return in.substr(0, end + 1);
}

std::vector<char> base64_decode(std::string_view b64) {
    std::vector<char> rv(beast::detail::base64::decoded_size(b64.size()));

    auto sizes = beast::detail::base64::decode(&rv[0], b64.data(), b64.size());
    rv.resize(sizes.first);

    return rv;
}

constexpr boost::urls::grammar::lut_chars custom_unreserved_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-._~/";

std::string url_encode(const std::string& str_to_encode) noexcept {
    auto encoded_string =
        boost::urls::encode(str_to_encode, custom_unreserved_chars);

    return encoded_string;
}

std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool to_bool(std::string str_to_eval) {
    std::istringstream is(lowercase(std::move(str_to_eval)));
    bool b;
    is >> std::boolalpha >> b;
    return b;
}

} // namespace uh::cluster
