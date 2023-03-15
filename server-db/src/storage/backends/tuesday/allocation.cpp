#include "allocation.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

tuesday_allocation::tuesday_allocation(tree& t)
    : m_tree(t),
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

    return { hash, m_size };
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
