#include "ut_tree.h"

#include <cassert>


namespace uh::trees
{

// ---------------------------------------------------------------------

std::list<const ultitree_tree::treenode*>
ultitree_tree::treenode::insert(std::span<const char> buffer)
{
    if (buffer.empty())
    {
        return {};
    }

    auto childs = children.find(*buffer.begin());
    if (childs == children.end())
    {
        return { insert_child(buffer) };
    }

    std::vector<treenode>& child_vec = childs->second;
    auto [best, max_common] = find_most_common(child_vec, buffer, ultitree_tree::MINIMUM_FRAGMENT);

    if (best == child_vec.end())
    {
        return { insert_child(buffer) };
    }

    if (best->data.size() == max_common)
    {
        auto rv = best->insert(buffer.subspan(max_common));
        rv.push_front(&*best);
        return rv;
    }

    auto best_remains = best->data.subspan(max_common);
    auto buffer_remains = buffer.subspan(max_common);

    best->data = best->data.subspan(0, max_common);
    best->insert_child(best_remains);
    auto last = best->insert_child(buffer_remains);

    return { &*best, last };
}

// ---------------------------------------------------------------------

ultitree_tree::treenode* ultitree_tree::treenode::insert_child(std::span<const char> buffer)
{
    auto& vec = children[*buffer.begin()];
    return &*vec.insert(vec.end(), { buffer });
}

// ---------------------------------------------------------------------

ultitree_tree::ultitree_tree()
    : m_root(std::make_unique<treenode>())
{
}

// ---------------------------------------------------------------------

std::list<const ultitree_tree::treenode*> ultitree_tree::insert(std::span<const char> buffer)
{
    return m_root->insert(buffer);
}

// ---------------------------------------------------------------------

} // namespace uh::trees
