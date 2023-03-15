#include "devices.h"


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

tuesday_write_device::tuesday_write_device(tree& t, tree_path& path, std::size_t& alloc_size)
    : m_tree(t),
      m_path(path),
      m_alloc_size(alloc_size)
{
}

// ---------------------------------------------------------------------

std::streamsize tuesday_write_device::write(std::span<const char> buffer)
{
    std::streamsize rv = buffer.size();
    while (!buffer.empty())
    {
        auto mr = m_tree.m_root.match(buffer);

        if (mr.path.empty())
        {
            auto node = m_tree.allocate(buffer);
            m_alloc_size += buffer.size();

            if (!m_path.empty())
            {
                m_path.back()->reference(node);
            }

            m_path.push_back(node);
            m_tree.m_root.reference(node);
            break;
        }

        auto& last = mr.path.back();
        if (mr.split_at < last->size())
        {
            auto split = last->split(mr.split_at);
            mr.path.pop_back();
            mr.path.push_back(split.first);
        }

        m_path.insert(m_path.end(), mr.path.begin(), mr.path.end());
        buffer = buffer.subspan(mr.length);
    }

    return rv;
}

// ---------------------------------------------------------------------

std::streamsize tuesday_write_device::read(std::span<char> buffer)
{
    throw std::runtime_error("reading not supported");
}

// ---------------------------------------------------------------------

bool tuesday_write_device::valid() const
{
    return true;
}

// ---------------------------------------------------------------------

tuesday_read_device::tuesday_read_device(const tree_path& p)
    : m_p(p),
      m_offset(0u)
{
}

// ---------------------------------------------------------------------

std::streamsize tuesday_read_device::write(std::span<const char> buffer)
{
    throw std::runtime_error("writing not supported");
}

// ---------------------------------------------------------------------

std::streamsize tuesday_read_device::read(std::span<char> buffer)
{
    std::streamsize rv = 0;

    while (!m_p.empty() && !buffer.empty())
    {
        auto first = m_p.front();

        auto count = std::min(buffer.size(), first->size() - m_offset);

        std::memcpy(buffer.data(), first->data() + m_offset, count);
        rv += count;

        buffer = buffer.subspan(count);
        if (count + m_offset >= first->size())
        {
            m_p.pop_front();
            m_offset = 0u;
        }
        else
        {
            m_offset += count;
        }
    }

    return rv;
}

// ---------------------------------------------------------------------

bool tuesday_read_device::valid() const
{
    return !m_p.empty();
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
