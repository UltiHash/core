#include "allocation.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

tuesday_allocation::tuesday_allocation(
    tree& t,
    std::size_t allocated,
    std::atomic<std::size_t>& used_size)
    : m_tree(t),
      m_allocated(allocated),
      m_used_size(used_size),
      m_device(m_tree, m_path, m_size),
      m_sha(m_device)
{
}

// ---------------------------------------------------------------------

io::device& tuesday_allocation::device()
{
    return m_sha;
}

// ---------------------------------------------------------------------

uh::protocol::block_meta_data tuesday_allocation::persist()
{
    auto hash = m_sha.finalize();
    m_tree.m_index[hash] = m_path;

    std::size_t saved = m_allocated - m_size;
    m_used_size -= saved;

    return { hash, m_size };
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
