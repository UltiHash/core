#include <io/test_generator.h>


namespace uh::io::test
{

// ---------------------------------------------------------------------

generator::generator(std::span<char> data, std::size_t chunk_size)
    : m_size(data.size())
{
    bool vec = false;
    while (!data.empty())
    {
        auto size = std::min(chunk_size, data.size());
        auto chunk = data.subspan(0, size);

        if (vec)
        {
            m_chunks.push_back(std::vector<char>(chunk.begin(), chunk.end()));
        }
        else
        {
            m_chunks.push_back(chunk);
        }

        vec = !vec;
        data = data.subspan(size);
    }
}

// ---------------------------------------------------------------------

std::optional<data_chunk> generator::next()
{
    if (m_chunks.empty())
    {
        return std::nullopt;
    }

    auto front = m_chunks.front();
    m_chunks.pop_front();
    return front;
}

// ---------------------------------------------------------------------

std::optional<std::size_t> generator::size()
{
    return m_size;
}

// ---------------------------------------------------------------------

} // namespace uh::io::test
