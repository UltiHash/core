#ifndef TREES_BPLUS_H
#define TREES_BPLUS_H

#include "fragment.h"
#include "indirect.h"

#include <list>
#include <set>
#include <span>


namespace uh::trees
{

// ---------------------------------------------------------------------

class ultitree_set
{
public:

    hash insert(std::span<const char> buffer);

    std::string find(const hash& h);

private:
    std::set<fragment>::iterator greatest_less_than(std::span<const char> buffer);
    std::set<fragment>::iterator least_greater_or_equal(std::span<const char> buffer);

    bool insert_at(std::set<fragment>::iterator pos,
                   std::span<const char>& buffer,
                   std::list<const fragment*>& path);

    std::set<fragment> m_fragments;
    indirection m_ind;
    std::size_t m_minimum_fragment = 2;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
