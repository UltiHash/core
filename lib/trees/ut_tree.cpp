#include "ut_tree.h"

#include <cassert>


namespace uh::trees
{

// ---------------------------------------------------------------------

path ultitree_tree::treenode::insert(std::span<const char> buffer, indirection& ind)
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
        auto rv = best->insert(buffer.subspan(max_common), ind);
        rv.push_front(&*best);
        return rv;
    }

    auto best_remains = best->data.subspan(max_common);
    auto buffer_remains = buffer.subspan(max_common);

    best->data = best->data.subspan(0, max_common);
    auto best2nd = best->insert_child(best_remains);
    auto last = best->insert_child(buffer_remains);

    ind.update(&*best, { &*best, best2nd });

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

hash ultitree_tree::insert(std::span<const char> buffer)
{
    return m_ind.add_path(m_root->insert(buffer, m_ind));
}

// ---------------------------------------------------------------------

std::string ultitree_tree::find(const hash& h)
{
    return join(m_ind.to_path(h));
}

// ---------------------------------------------------------------------

} // namespace uh::trees
