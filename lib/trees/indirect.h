#ifndef TREES_INDIRECT_H
#define TREES_INDIRECT_H

#include "fragment.h"
#include <map>
#include <string>


namespace uh::trees
{

// ---------------------------------------------------------------------

typedef std::string hash;

// ---------------------------------------------------------------------

class indirection
{
public:

    /**
     * Add a path and return a hash to it.
     */
    hash add_path(const path& p);

    /**
     * Return the path for a hash.
     */
    const path& to_path(const hash& h) const;

    /**
     * Replace the occurences of `value` in all paths with `replace`.
     */
    void update(typename path::value_type value, const path& replace);

private:
    std::map<hash, path> m_paths;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
