#ifndef TREES_BPLUS_H
#define TREES_BPLUS_H

#include <list>
#include <set>
#include <span>


namespace uh::trees
{

// ---------------------------------------------------------------------

struct fragment
{
    std::span<const char> data;

    bool operator<(const fragment& other) const;
};

// ---------------------------------------------------------------------

class bplus
{
public:

    /**
     * Insert a buffer into the bplus and return a list of fragments to
     * recover the buffer.
     */
    std::list<const fragment*> insert(std::span<const char> buffer);

private:
    std::set<fragment>::iterator greatest_less_than(std::span<const char> buffer);
    std::set<fragment>::iterator least_greater_or_equal(std::span<const char> buffer);

    bool insert_at(std::set<fragment>::iterator pos,
                   std::span<const char>& buffer,
                   std::list<const fragment*>& path);

    std::set<fragment> m_fragments;
    std::size_t m_minimum_fragment = 2;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
