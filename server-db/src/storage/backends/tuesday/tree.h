#ifndef SERVER_DB_STORAGE_BACKENDS_TUESDAY_TREE_H
#define SERVER_DB_STORAGE_BACKENDS_TUESDAY_TREE_H

#include <protocol/common.h>

#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <vector>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

struct treenode;

// ---------------------------------------------------------------------

typedef std::list<std::shared_ptr<treenode>> tree_path;

// ---------------------------------------------------------------------

class treenode
{
public:
    treenode() = default;
    treenode(std::span<char> buffer);
    std::size_t size() const { return m_buffer.size(); }
    char* data() const { return m_buffer.data(); }

    /**
     * Add a successor reference to `node`.
     */
    void reference(std::shared_ptr<treenode> node);
private:
    friend class tree;
    friend class tuesday_write_device;
    struct match_result
    {
        tree_path path;
        std::size_t split_at;
        std::size_t length = 0u;
    };

    match_result match(std::span<const char> buffer);

    std::pair<std::shared_ptr<treenode>, std::shared_ptr<treenode>> split(size_t pos);

    std::map<char, std::set<std::shared_ptr<treenode>>> m_succs;
    std::span<char> m_buffer;
};

// ---------------------------------------------------------------------

class tree
{
public:
    std::shared_ptr<treenode> allocate(std::span<const char> buffer);

    uh::protocol::blob put(std::span<char> buffer);
    std::vector<char> get(const uh::protocol::blob& hash) const;

    const tree_path& get_path(const uh::protocol::blob& hash) const;

private:
    friend class tuesday_write_device;
    friend class tuesday_allocation;

    treenode m_root;
    std::map<uh::protocol::blob, tree_path> m_index;
    std::list< std::vector<char> > m_allocations;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
