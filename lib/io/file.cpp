#include "file.h"

#include "util/exception.h"

namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode)
    : m_path(path),m_mode(mode),m_io(std::fstream(path,mode))
{
    m_io.exceptions( std::ifstream::badbit | std::ifstream::failbit );
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

void file::seek(std::streamoff off, const std::ios_base::seekdir whence)
{
    if(m_mode & std::ios_base::in){
        m_io.seekg(off,whence);
    }
    else{
        m_io.seekp(off,whence);
    }
}

// ---------------------------------------------------------------------

std::filesystem::path file::path()
{
    return m_path;
}

// ---------------------------------------------------------------------

} // namespace uh::io
