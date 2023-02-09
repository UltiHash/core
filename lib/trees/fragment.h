#ifndef TREES_FRAGMENT_H
#define TREES_FRAGMENT_H

#include <span>

namespace uh::trees
{

// ---------------------------------------------------------------------

struct fragment
{
    std::span<const char> data;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
