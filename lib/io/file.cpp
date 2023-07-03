#include "file.h"

#include "util/exception.h"

namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode)
    : m_io(path, mode),
      m_path(path)
{
    m_io.exceptions(std::ifstream::badbit | std::ifstream::failbit);
}

// ---------------------------------------------------------------------

std::streamsize file::write(std::span<const char> buffer)
{
    m_io.write(buffer.data(), buffer.size());
    return buffer.size();
}

// ---------------------------------------------------------------------

std::streamsize file::read(std::span<char> buffer)
{
    m_io.read(buffer.data(), buffer.size());
    return m_io.gcount();
}

// ---------------------------------------------------------------------

bool file::valid() const
{
    return m_io.good();
}

// ---------------------------------------------------------------------

void file::seek(std::streamoff off, std::ios_base::seekdir whence)
{
    /* For std::fstream (and relatives), `seekg` and `seekp` are changing
     * the same pointer. From https://en.cppreference.com/w/cpp/io/basic_filebuf:
     *
     *   std::basic_filebuf is a std::basic_streambuf whose associated
     *   character sequence is a file. Both the input sequence and the
     *   output sequence are associated with the same file, and a joint
     *   file position is maintained for both operations.
     *
     * If we seek with std::ios_base::cur this means we would move the
     * pointer twice, if also calling seekp after seekg. To avoid wrong
     * positioning, we only call `seekg`, relying on basic_filebuf to
     * adjust the put-pointer as well.
     */
    m_io.seekg(off, whence);
}

// ---------------------------------------------------------------------

std::filesystem::path file::path()
{
    return m_path;
}

// ---------------------------------------------------------------------

std::size_t file::size()
{
    auto pos = m_io.tellg();
    m_io.seekg(0, std::ios::end);

    auto rv = m_io.tellg();
    m_io.seekg(pos, std::ios::beg);

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::io
