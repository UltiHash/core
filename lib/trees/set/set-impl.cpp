#include "bplus.h"

#include <algorithm>
#include <cassert>


namespace uh::trees
{

// ---------------------------------------------------------------------

bool fragment::operator<(const fragment& other) const
{
    return std::lexicographical_compare(
        data.begin(), data.end(),
        other.data.begin(), other.data.end());
}

// ---------------------------------------------------------------------

std::list<const fragment*> bplus::insert(std::span<const char> buffer)
{
    std::list<const fragment*> rv;

    while (!buffer.empty())
    {
        auto glt = greatest_less_than(buffer);
        if (glt != m_fragments.end() && insert_at(glt, buffer, rv))
        {
            continue;
        }

        auto lge = least_greater_or_equal(buffer);
        if (lge != m_fragments.end() && insert_at(lge, buffer, rv))
        {
            continue;
        }

        auto it = m_fragments.insert(fragment{ buffer });
        assert(it.second);

        rv.push_back(&*it.first);
        break;
    }

    return rv;
}

// ---------------------------------------------------------------------

bool bplus::insert_at(std::set<fragment>::iterator pos,
                      std::span<const char>& buffer,
                      std::list<const fragment*>& path)
{
    auto mismatch = std::mismatch(pos->data.begin(), pos->data.end(),
                                  buffer.begin(), buffer.end());

    auto length = std::distance(pos->data.begin(), mismatch.first);
    if (length < m_minimum_fragment)
    {
        return false;
    }

    auto remains_length = std::distance(mismatch.second, buffer.end());
    if (remains_length > 0 && remains_length < m_minimum_fragment)
    {
        return false;
    }

    if (mismatch.first == pos->data.end())
    {
        buffer = buffer.subspan(length);
        path.push_back(&*pos);
        return true;
    }

    auto first = m_fragments.insert(fragment{ pos->data.subspan(0, length) });
    auto second = m_fragments.insert(fragment{ pos->data.subspan(length) });

    assert(first.second && second.second);

    path.push_back(&*first.first);
    buffer = buffer.subspan(length);
    return true;
}

// ---------------------------------------------------------------------

std::set<fragment>::iterator bplus::greatest_less_than(std::span<const char> buffer)
{
    if (m_fragments.empty())
    {
        return m_fragments.end();
    }

    auto it = m_fragments.lower_bound(fragment{ buffer });
    if (it == m_fragments.end())
    {
        return (++m_fragments.rbegin()).base();
    }

    if (it == m_fragments.begin())
    {
        return m_fragments.end();
    }

    --it;
    return it;
}

// ---------------------------------------------------------------------

std::set<fragment>::iterator bplus::least_greater_or_equal(std::span<const char> buffer)
{
    return m_fragments.lower_bound(fragment{ buffer });
}

// ---------------------------------------------------------------------

} // namespace uh::trees
