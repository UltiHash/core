#include "strings.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/url.hpp>
#include <boost/url/encode.hpp>
#include <cctype>
#include <charconv>
#include <sstream>

using namespace boost;

namespace uh::cluster {

namespace {

int xvalue(char ch) {
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }

    return ch - '0';
}

} // namespace

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

std::vector<char> base64_encode(std::string_view plain) {
    std::vector<char> rv(beast::detail::base64::encoded_size(plain.size()));

    auto size =
        beast::detail::base64::encode(&rv[0], plain.data(), plain.size());
    rv.resize(size);

    return rv;
}

std::string erase_all(std::string haystack, const std::string& characters) {
    std::string rv;
    rv.reserve(haystack.size());

    for (auto ch : haystack) {
        if (characters.find(ch) == std::string::npos) {
            rv += ch;
        }
    }

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

std::string uri_encode(const std::string& str,
                       const std::string& also_encode) noexcept {
    std::string rv;
    for (auto it = str.begin(); it != str.end(); ++it) {
        if ((*it >= 'A' && *it <= 'Z') || (*it >= 'a' && *it <= 'z') ||
            (*it >= '0' && *it <= '9') || *it == '-' || *it == '.' ||
            *it == '_' || *it == '~' ||
            also_encode.find(*it) != std::string::npos) {

            rv += *it;
            continue;
        }

        rv += '%' + to_hexu(*it);
    }

    return rv;
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

bool equals_nocase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (auto i = 0ull; i < a.size(); ++i) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }

    return true;
}

std::string unhex(std::string in) {
    if (in.size() % 2 != 0) {
        throw std::invalid_argument("string size must be even");
    }

    std::transform(in.begin(), in.end(), in.begin(), ::tolower);

    std::string rv;
    for (std::size_t pos = 0ull; pos < in.size(); pos += 2) {
        if (!std::isxdigit(in[pos]) || !std::isxdigit(in[pos + 1])) {
            throw std::invalid_argument("string contains non-hex characters");
        }

        rv += static_cast<char>(xvalue(in[pos]) << 8 | xvalue(in[pos + 1]));
    }

    return rv;
}

std::size_t stoul(std::string_view s, std::size_t* pos, int base) {

    std::size_t rv{};
    auto r = std::from_chars(s.begin(), s.end(), rv, base);

    if (r.ec != std::errc()) {
        throw std::runtime_error("Group state value out of range");
    }

    if (pos) {
        *pos = std::distance(s.begin(), r.ptr);
    }

    return rv;
}

std::size_t ctoul(const char& c, std::size_t* pos, int base) {
    return stoul(std::string_view(&c, 1), pos, base);
}

} // namespace uh::cluster
