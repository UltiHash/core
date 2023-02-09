#ifndef TREES_ULTITREE_H
#define TREES_ULTITREE_H

#include "fragment.h"
#include "indirect.h"

#include <list>


namespace uh::trees
{

// ---------------------------------------------------------------------

class ultitree_list
{
public:

    hash insert(std::span<const char> buffer);
    std::string find(const hash& h);

private:
    std::list<fragment> m_fragments;
    indirection m_ind;
    std::size_t m_minimum_fragment = 2;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
