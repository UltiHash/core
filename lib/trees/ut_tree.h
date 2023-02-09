#ifndef TREES_UT_TREE_H
#define TREES_UT_TREE_H

#include "fragment.h"

#include <list>
#include <map>
#include <memory>
#include <vector>


namespace uh::trees
{

// ---------------------------------------------------------------------

class ultitree_tree
{
public:

    struct treenode : public fragment
    {
        std::map<char, std::vector<treenode>> children;

        path insert(std::span<const char> buffer);
        treenode* insert_child(std::span<const char> buffer);
    };

    ultitree_tree();

    path insert(std::span<const char> buffer);

    static constexpr std::size_t MINIMUM_FRAGMENT = 2;
private:
    std::unique_ptr<treenode> m_root;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
