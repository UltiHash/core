#include "user.h"

#include <cstdlib>


namespace uh::util
{

// ---------------------------------------------------------------------

std::filesystem::path user::home()
{
    auto* value = std::getenv("HOME");
    if (value != nullptr)
    {
        return value;
    }

    return {};
}

// ---------------------------------------------------------------------

} // namespace uh::util
