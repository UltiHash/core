#include "common/utils/random.h"


namespace uh::cluster
{

// ---------------------------------------------------------------------

std::string random_string(std::size_t length, const std::string& chars)
{
    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, chars.size());

    std::string s;

    while (s.size() < length)
    {
        s += 97 + chars[pick(rg)] % 25;
    }

    return s;
}

// ---------------------------------------------------------------------

} // namespace uh::cluster
