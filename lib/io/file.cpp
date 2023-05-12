#include "file.h"


namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_io(path)
{
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode)
    : m_io(path, mode)
{
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

void file::seek(off64_t pos) {
    m_io.seekg(pos);
}

void file::seek(off64_t off,std::ios_base::seekdir way) {
    m_io.seekg(off,way);
}

// ---------------------------------------------------------------------

} // namespace uh::io
