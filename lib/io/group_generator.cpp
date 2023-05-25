#include "group_generator.h"


namespace uh::io
{

// ---------------------------------------------------------------------

void group_generator::append(std::unique_ptr<data_generator>&& dg)
{
    m_generators.emplace_back(std::move(dg));
}

// ---------------------------------------------------------------------

std::optional<data_chunk> group_generator::next()
{
    while (!m_generators.empty())
    {
        auto rv = m_generators.front()->next();
        if (rv)
        {
            return rv;
        }

        m_generators.pop_front();
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------

std::size_t group_generator::size() const
{
    std::size_t rv = 0;

    for (const auto& g : m_generators)
    {
        rv += g->size();
    }

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::io
