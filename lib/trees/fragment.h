#ifndef TREES_FRAGMENT_H
#define TREES_FRAGMENT_H

#include <list>
#include <span>
#include <string>


namespace uh::trees
{

// ---------------------------------------------------------------------

struct fragment
{
    std::span<const char> data;

    std::string to_string() const;
    bool operator<(const fragment& other) const;
};

// ---------------------------------------------------------------------

typedef std::list<const fragment*> path;

// ---------------------------------------------------------------------

std::string join(const path& p);

// ---------------------------------------------------------------------

template <typename container>
std::pair<typename container::iterator, typename container::size_type>
find_most_common(container& c, std::span<const char> buffer, std::size_t min_size)
{
    typename container::iterator best = c.end();
    typename container::size_type max = 0u;
    for (auto it = c.begin(); it != c.end(); ++it)
    {
        auto mismatch = std::mismatch(buffer.begin(), buffer.end(),
                                      it->data.begin(), it->data.end());

        auto common_length = std::distance(buffer.begin(), mismatch.first);
        if (common_length < min_size)
        {
            continue;
        }

        if (buffer.size() - common_length < min_size)
        {
            continue;
        }

        if (it->data.size() - common_length < min_size)
        {
            continue;
        }

        if (common_length > max)
        {
            max = common_length;
            best = it;
        }
    }

    return std::make_pair(best, max);
}

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
