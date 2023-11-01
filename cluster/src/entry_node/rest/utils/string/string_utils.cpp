#include <algorithm>
#include <cstring>
#include "string_utils.h"
#include <boost/url/encode.hpp>
#include <boost/url.hpp>

namespace uh::cluster::rest::utils
{

    constexpr
    boost::urls::grammar::lut_chars
            custom_unreserved_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "-._~/";

    std::string string_utils::to_lower(const char* source)
    {
        std::string copy;
        size_t source_length = std::strlen(source);
        copy.resize(source_length);
        std::transform(source, source + source_length, copy.begin(), [](unsigned char c) { return (char)::tolower(c); });

        return copy;
    }

    bool string_utils::is_same(const char* string1, const char* string2)
    {
        return ( strcmp(string1, string2) == 0 );
    }

    std::string string_utils::URL_encode(const std::string& str_to_encode)
    {
        auto encoded_string = boost::urls::encode(str_to_encode, custom_unreserved_chars);

        return encoded_string;
    }

} // uh::cluster::rest::utils
