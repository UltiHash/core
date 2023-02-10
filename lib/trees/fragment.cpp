#include "fragment.h"


namespace uh::trees
{

// ---------------------------------------------------------------------

std::string fragment::to_string() const
{
    return std::string(data.begin(), data.end());
}

// ---------------------------------------------------------------------

bool fragment::operator<(const fragment& other) const
{
    return std::lexicographical_compare(
        data.begin(), data.end(),
        other.data.begin(), other.data.end());
}

// ---------------------------------------------------------------------

std::string join(const path& p)
{
    std::string rv;

    for (const auto& i : p)
    {
        rv += i->to_string();
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::trees
