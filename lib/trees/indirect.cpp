#include "indirect.h"

#include <algorithm>


namespace uh::trees
{

// ---------------------------------------------------------------------

hash indirection::add_path(const path& p)
{
    hash rv = join(p);

    m_paths[rv] = p;

    return rv;
}

// ---------------------------------------------------------------------

const path& indirection::to_path(const hash& h) const
{
    auto it = m_paths.find(h);

    if (it == m_paths.end())
    {
        throw "hash unknown";
    }

    return it->second;
}

// ---------------------------------------------------------------------

void indirection::update(typename path::value_type value, const path& replace)
{
    for (auto& pair : m_paths)
    {
        path& p = pair.second;

        auto it = std::find(p.begin(), p.end(), value);
        while (it != p.end())
        {
            p.insert(it, replace.begin(), replace.end());
            it = p.erase(it);
            it = std::find(it, p.end(), value);
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::trees
