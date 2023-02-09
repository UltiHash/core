#include "ultitree.h"

#include <algorithm>
#include <cassert>


namespace uh::trees
{

// ---------------------------------------------------------------------

std::list<const fragment*> ultitree_list::insert(std::span<const char> buffer)
{
    std::list<const fragment*> rv;

    while (!buffer.empty())
    {
        auto match = find_most_common(buffer);
        if (match.first == m_fragments.end())
        {
            auto last = m_fragments.insert(m_fragments.end(), fragment{ buffer });
            rv.push_back(&*last);
            break;
        }

        if (match.second >= buffer.size())
        {
            auto frag = m_fragments.insert(m_fragments.end(), fragment{ buffer });
            rv.push_back(&*frag);
            break;
        }

        fragment& f = *match.first;
        auto head = m_fragments.insert(m_fragments.end(), fragment{ f.data.subspan(0, match.second) });
        m_fragments.push_back(fragment{ f.data.subspan(match.second) });

        rv.push_back(&*head);
        buffer = buffer.subspan(match.second);
    }

    return rv;
}

// ---------------------------------------------------------------------

std::pair<std::list<fragment>::iterator, std::size_t>
ultitree_list::find_most_common(std::span<const char> buffer)
{
    std::size_t length = 0;
    std::list<fragment>::iterator rv = m_fragments.end();

    for (auto it = m_fragments.begin(); it != m_fragments.end(); ++it)
    {
        auto mismatch = std::mismatch(buffer.begin(), buffer.end(),
                                      it->data.begin(), it->data.end());

        std::size_t common_length = std::distance(buffer.begin(), mismatch.first);

        if (common_length < m_minimum_fragment)
        {
            continue;
        }

        if (common_length > length)
        {
            length = common_length;
            rv = it;
        }
    }

    return std::make_pair(rv, length);
}

// ---------------------------------------------------------------------

} // namespace uh::trees
