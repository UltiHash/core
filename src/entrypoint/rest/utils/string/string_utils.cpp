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

    bool string_utils::is_bool(const std::string& str_to_eval)
    {
        if (str_to_eval == "true" || str_to_eval == "TRUE" || str_to_eval == "True")
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    std::string string_utils::URL_encode(const std::string& str_to_encode)
    {
        auto encoded_string = boost::urls::encode(str_to_encode, custom_unreserved_chars);

        return encoded_string;
    }

    std::vector<std::string>::iterator string_utils::find_lexically_closest(std::vector<std::string>& strings,
                                                                    const std::string& compareTo)
    {
        if (strings.empty())
        {
            return strings.end();
        }

        if (compareTo.empty())
        {
            return strings.begin();
        }

        auto lexicalCompare = [](const std::string& a, const std::string& b)
        {
            return a < b;
        };

        auto nextDifferentItr = std::lower_bound(strings.begin(), strings.end(), compareTo, lexicalCompare);

        if (nextDifferentItr != strings.end() && *nextDifferentItr == compareTo)
        {
            ++nextDifferentItr;
        } // TODO REVIEW THIS ONE MORE TIME, SOME TESTS FAIL BECAUSE OF REMOVING IT BUT CONCEPTUALLY IT SHOULDN'T

        return nextDifferentItr;
    }

} // uh::cluster::rest::utils
