#include "tree.h"

#include <hash/sha512.h>


using namespace uh::hash;

namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

std::shared_ptr<treenode> tree::allocate(std::span<const char> buffer)
{
    std::lock_guard<std::mutex> guard(m_mtx_allocations);
    m_allocations.emplace_back(buffer.begin(), buffer.end());

    auto& alloc = m_allocations.back();

    return std::make_shared<treenode>(std::span<char>{alloc.begin(), alloc.end()});
}

// ---------------------------------------------------------------------

uh::protocol::blob tree::put(std::span<char> buffer)
{
    tree_path path;

    auto hash = sha512_digest(buffer);

    while (!buffer.empty())
    {
        auto mr = m_root.match(buffer);

        if (mr.path.empty())
        {
            auto node = allocate(buffer);

            if (!path.empty())
            {
                path.back()->reference(node);
            }

            path.push_back(node);
            break;
        }

        auto& last = mr.path.back();
        if (mr.split_at < last->size())
        {
            auto split = last->split(mr.split_at);
            mr.path.pop_back();
            mr.path.push_back(split.first);
        }
    }

    add_index(hash, path);
    return hash;
}

// ---------------------------------------------------------------------

std::vector<char> tree::get(const uh::protocol::blob& hash) const
{
    std::vector<char> rv;

    for (auto node : get_path(hash))
    {
        rv.insert(rv.end(), node->m_buffer.begin(), node->m_buffer.end());
    }

    return rv;
}

// ---------------------------------------------------------------------

const tree_path& tree::get_path(const uh::protocol::blob& hash) const
{
    std::lock_guard<std::mutex> guard(m_mtx_hashes);

    auto iter = m_hashes.find(hash);
    if (iter == m_hashes.end())
    {
        throw std::runtime_error("unknown hash");
    }

    return iter->second;
}

// ---------------------------------------------------------------------

treenode::treenode(std::span<char> buffer)
    : m_buffer(buffer)
{
}

// ---------------------------------------------------------------------

void treenode::reference(std::shared_ptr<treenode> node)
{
    m_succs[node->m_buffer[0]].insert(node);
}

// ---------------------------------------------------------------------

treenode::match_result treenode::match(std::span<const char> buffer)
{
    auto succs = m_succs.find(buffer[0]);
    if (succs == m_succs.end())
    {
        return {};
    }

    tree_path longest;
    size_t split_at = 0u;
    size_t length = 0u;

    for (const auto& s : succs->second)
    {
        auto mm = std::mismatch(buffer.begin(), buffer.end(),
                                s->m_buffer.begin(), s->m_buffer.end());

        if (mm.first == buffer.end())
        {
            return
            {
                { s },
                static_cast<size_t>(std::distance(s->m_buffer.begin(), mm.second)),
                buffer.size()
            };
        }

        if (mm.second == s->m_buffer.end())
        {
            auto sub_mr = s->match(buffer.subspan(s->m_buffer.size()));
            if (length <= sub_mr.length)
            {
                longest = sub_mr.path;
                longest.push_front(s);
                split_at = sub_mr.split_at;
                length = sub_mr.length + s->m_buffer.size();
            }

            continue;
        }

        if (longest.empty())
        {
            auto dist = std::distance(buffer.begin(), mm.first);
            if (dist != 0)
            {
                longest = { s };
                split_at = dist;
                length = dist;
            }
        }
    }

    return { longest, split_at, length };
}

// ---------------------------------------------------------------------

std::pair<std::shared_ptr<treenode>, std::shared_ptr<treenode>> treenode::split(size_t pos)
{
    auto first = std::make_shared<treenode>(m_buffer.subspan(0, pos));
    auto second = std::make_shared<treenode>(m_buffer.subspan(pos));

    first->reference(second);
    second->m_succs = m_succs;

    return std::make_pair(first, second);
}

// ---------------------------------------------------------------------

void tree::add_index(const uh::protocol::blob& b, const tree_path& path)
{
    std::lock_guard<std::mutex> guard(m_mtx_hashes);
    m_hashes[b] = path;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
