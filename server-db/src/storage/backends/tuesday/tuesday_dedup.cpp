#include "tuesday_dedup.h"

#include <atomic>

#include "allocation.h"
#include "devices.h"
#include "tree.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

struct tuesday_dedup::impl
{
    impl(metrics::storage_metrics& metrics, std::size_t alloc_size);

    metrics::storage_metrics& m_metrics;
    std::size_t m_alloc_size;
    std::atomic<std::size_t> m_used_size;

    tree m_tree;
};

// ---------------------------------------------------------------------

tuesday_dedup::impl::impl(metrics::storage_metrics& metrics, std::size_t alloc_size)
    : m_metrics(metrics),
      m_alloc_size(alloc_size),
      m_used_size(0u)
{
}

// ---------------------------------------------------------------------

tuesday_dedup::tuesday_dedup(metrics::storage_metrics& metrics, std::size_t alloc_size)
    : m_impl(std::make_unique<impl>(metrics, alloc_size))
{
}

// ---------------------------------------------------------------------

tuesday_dedup::~tuesday_dedup() = default;

// ---------------------------------------------------------------------

std::unique_ptr<io::device> tuesday_dedup::read_block(const uh::protocol::blob& hash)
{
    return std::make_unique<tuesday_read_device>(m_impl->m_tree.get_path(hash));
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> tuesday_dedup::allocate(std::size_t size)
{
    while (true)
    {
        std::size_t used = m_impl->m_used_size;
        if (m_impl->m_alloc_size - used <= size)
        {
            THROW(util::exception, "out of space");
        }

        auto new_val = used + size;
        if (m_impl->m_used_size.compare_exchange_weak(used, new_val))
        {
            break;
        }
    }

    try
    {
        return std::make_unique<tuesday_allocation>(m_impl->m_tree);
    }
    catch (...)
    {
        m_impl->m_used_size -= size;
        throw;
    }
}

// ---------------------------------------------------------------------

size_t tuesday_dedup::free_space()
{
    return m_impl->m_alloc_size - m_impl->m_used_size;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
