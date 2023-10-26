#include <algorithm>
#include <cstring>
#include "string_utils.h"

namespace uh::cluster::rest::utils
{

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

    std::string URL_encode()
    {
//        std::string escaped;
//        escaped.fill('0');
//        escaped << std::hex << std::uppercase;
//
//        size_t unsafeLength = strlen(unsafe);
//        for (auto i = unsafe, n = unsafe + unsafeLength; i != n; ++i)
//        {
//            char c = *i;
//            if (IsAlnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
//            {
//                escaped << (char)c;
//            }
//            else
//            {
//                //this unsigned char cast allows us to handle unicode characters.
//                escaped << '%' << std::setw(2) << int((unsigned char)c) << std::setw(0);
//            }
//        }
//
//        return escaped.str();
    }

} // uh::cluster::rest::utils
