#include "device.h"


namespace uh::io
{

namespace
{

// ---------------------------------------------------------------------

constexpr std::streamsize BUFFER_SIZE = 1024 * 1024;

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

std::streamsize device::write_range(data_generator& generator)
{
    std::streamsize rv = 0;

    for (auto chunk = generator.next(); chunk; chunk = generator.next())
    {
        rv += uh::io::write(*this, chunk->data());
    }

    return rv;
}

// ---------------------------------------------------------------------

std::vector<char> read_to_buffer(device& dev, std::streamsize chunk_size)
{
    std::streamsize read = 0;
    std::streamsize pos = 0;
    std::vector<char> buffer;

    do
    {
        buffer.resize(buffer.size() + chunk_size);

        read = dev.read({ buffer.begin() + pos, buffer.size() - pos });
        pos += read;
        buffer.resize(pos);
    }
    while (read != 0);

    return buffer;
}

// ---------------------------------------------------------------------

std::size_t write_from_buffer(
    device& dev,
    std::span<char> buffer)
{
    return dev.write(buffer);
}

// ---------------------------------------------------------------------

std::size_t copy(device& d, std::ostream& out)
{
    std::array<char, BUFFER_SIZE> buffer;
    std::size_t rv = 0;

    while (d.valid())
    {
        auto read = d.read(buffer);
        rv += read;
        if (!read)
        {
            break;
        }
        out.write(buffer.data(), read);
    }

    return rv;
}

// ---------------------------------------------------------------------

std::size_t copy(device& in, device& out)
{
    std::array<char, BUFFER_SIZE> buffer;
    std::size_t rv = 0;

    while (in.valid())
    {
        std::size_t read = in.read(buffer);
        rv += read;
        if (!read)
        {
            break;
        }
        out.write({ buffer.data(), read });
    }

    return rv;
}

// ---------------------------------------------------------------------

std::streamsize write(io::device& dev, std::span<const char> buffer)
{
    std::size_t written = 0;

    const auto total = buffer.size();
    while (written < total)
    {
        written += dev.write({buffer.data() + written, total - written});
    }

    return written;
}

// ---------------------------------------------------------------------

std::streamsize read(io::device& dev, std::span<char> buffer)
{
    std::size_t reads = 0;

    const auto total = buffer.size();
    while (reads < total)
    {
        auto read = dev.read({buffer.data() + reads, total - reads});
        if (read == 0)
        {
            break;
        }

        reads += read;
    }

    return reads;
}

// ---------------------------------------------------------------------

} // namespace uh::io
