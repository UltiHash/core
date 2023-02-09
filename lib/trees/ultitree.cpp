#include "ultitree.h"

#include <algorithm>
#include <cassert>


namespace uh::trees
{

// ---------------------------------------------------------------------

hash ultitree_list::insert(std::span<const char> buffer)
{
    path rv;

    while (!buffer.empty())
    {
        auto [best, max_common] = find_most_common(m_fragments, buffer, m_minimum_fragment);
        if (best == m_fragments.end())
        {
            auto last = m_fragments.insert(m_fragments.end(), fragment{ buffer });
            rv.push_back(&*last);
            break;
        }

        if (max_common >= buffer.size())
        {
            auto frag = m_fragments.insert(m_fragments.end(), fragment{ buffer });
            rv.push_back(&*frag);
            break;
        }

        fragment& f = *best;
        auto head = m_fragments.insert(m_fragments.end(), fragment{ f.data.subspan(0, max_common) });
        auto tail = m_fragments.insert(m_fragments.end(), fragment{ f.data.subspan(max_common) });

        m_ind.update(&*best, { &*head, &*tail });
        m_fragments.erase(best);

        rv.push_back(&*head);
        buffer = buffer.subspan(max_common);
    }

    return m_ind.add_path(rv);
}

// ---------------------------------------------------------------------

std::string ultitree_list::find(const hash& h)
{
    return join(m_ind.to_path(h));
}

// ---------------------------------------------------------------------

} // namespace uh::trees
