#ifndef TREES_ULTITREE_H
#define TREES_ULTITREE_H

#include "fragment.h"

#include <list>


namespace uh::trees
{

// ---------------------------------------------------------------------

class ultitree_list
{
public:

    std::list<const fragment*> insert(std::span<const char> buffer);

private:
    std::list<fragment> m_fragments;
    std::size_t m_minimum_fragment = 2;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
