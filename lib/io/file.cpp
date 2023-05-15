#include "file.h"

#include "util/exception.h"

namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path),m_mode(std::ios_base::out)
{
    if(!std::filesystem::is_directory(path))
        m_io = std::fstream(path);
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode)
    : m_path(path),m_mode(mode)
{

    if(!std::filesystem::is_directory(path))
        m_io = std::fstream(path,mode);

    if (!m_io.is_open())
        throw std::runtime_error("Could not open the file!");

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

void file::seek(std::streamoff off, std::ios_base::seekdir whence) {
    auto cur_pos = m_io.tellg();
    if(m_mode & std::ios_base::in){
        m_io.seekg(off,whence);
        if(off > std::abs(m_io.tellg()-cur_pos))
            THROW(util::exception,"input seek was out of range; seek incomplete");
    }
    else{
        if(m_mode & std::ios_base::out){
            m_io.seekp(off,whence);
            if(off > std::abs(m_io.tellp()-cur_pos))
                THROW(util::exception,"output seek was out of range; seek incomplete");
        }
        else{
            THROW(util::exception,"file mode was not supported for seeking");
        }
    }
}

// ---------------------------------------------------------------------

std::filesystem::path file::path()
{
    return m_path;
}

// ---------------------------------------------------------------------

bool file::is_open() {
    return m_io.is_open();
}

// ---------------------------------------------------------------------

void file::close() {
    m_io.close();
}

// ---------------------------------------------------------------------

} // namespace uh::io
