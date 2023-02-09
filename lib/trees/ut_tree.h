#ifndef TREES_UT_TREE_H
#define TREES_UT_TREE_H

#include "fragment.h"
#include "indirect.h"

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

        path insert(std::span<const char> buffer, indirection& ind);
        treenode* insert_child(std::span<const char> buffer);
    };

    ultitree_tree();

    hash insert(std::span<const char> buffer);

    std::string find(const hash& h);

    static constexpr std::size_t MINIMUM_FRAGMENT = 2;
private:
    std::unique_ptr<treenode> m_root;
    indirection m_ind;
};

// ---------------------------------------------------------------------

} // namespace uh::trees

#endif
