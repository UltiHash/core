#ifndef TREES_ULTITREE_H
#define TREES_ULTITREE_H

#include <list>
#include <span>


namespace uh::trees
{

// ---------------------------------------------------------------------

struct fragment
{
    std::span<const char> data;
};

// ---------------------------------------------------------------------

class ultitree
{
public:

    /**
     * Insert a buffer into the ultitree and return a list of fragments to
     * recover the buffer.
     */
    std::list<const fragment*> insert(std::span<const char> buffer);

private:

    std::pair<std::list<fragment>::iterator, std::size_t>
    find_most_common(std::span<const char> buffer);

    std::list<fragment> m_fragments;
    std::size_t m_minimum_fragment = 2;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
