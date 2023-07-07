#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>


namespace uh::util
{

// ---------------------------------------------------------------------

template <typename Iterator>
std::string to_hex_string(Iterator begin, Iterator end)
{
    std::string hex_string;

    boost::algorithm::hex(begin, end, std::back_inserter(hex_string));
    boost::algorithm::to_lower(hex_string);

    return hex_string;
}

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
