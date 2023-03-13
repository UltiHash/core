#ifndef UTIL_RANDOM_H
#define UTIL_RANDOM_H

#include <string>


namespace uh::util
{

// ---------------------------------------------------------------------

std::string random_string(
    std::size_t length = 16,
    const std::string& chars = "0123456789abcdefghijklmnopqrstuvwyz");

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
