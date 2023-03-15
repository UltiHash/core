#ifndef SERVER_DB_STORAGE_BACKENDS_TUESDAY_ALLOCATION_H
#define SERVER_DB_STORAGE_BACKENDS_TUESDAY_ALLOCATION_H

#include <protocol/allocation.h>
#include <hash/sha512.h>

#include "devices.h"
#include "tree.h"

#include <atomic>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

class tuesday_allocation : public uh::protocol::allocation
{
public:
    tuesday_allocation(
        tree& t,
        std::size_t allocated,
        std::atomic<std::size_t>& used_size);

    virtual io::device& device() override;
    virtual uh::protocol::block_meta_data persist() override;

private:
    tree& m_tree;
    std::size_t m_allocated;
    std::atomic<std::size_t>& m_used_size;
    std::size_t m_size = 0;
    tree_path m_path;
    tuesday_write_device m_device;
    std::map<uh::protocol::blob, tree_path> index;
    hash::sha512 m_sha;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
