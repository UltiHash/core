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


} // uh::cluster::rest::utils
