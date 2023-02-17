#ifndef UTIL_USER_H
#define UTIL_USER_H

#include <filesystem>


namespace uh::util
{

// ---------------------------------------------------------------------

class user
{
public:
    static std::filesystem::path home();
};

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
