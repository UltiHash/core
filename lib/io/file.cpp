#include "file.h"

#include "util/exception.h"

namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path),m_mode(std::ios_base::in)
{
    if(!std::filesystem::is_directory(path))
        m_io = std::fstream(path,m_mode);
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

void file::seek(std::streamoff off, const std::ios_base::seekdir whence) {
    std::streampos cur_pos = m_io.tellg();
    std::streampos next_pos;

    std::streampos max_pos;

    if(m_mode & std::ios_base::in){
        m_io.seekg(0,std::ios_base::end);
        max_pos = m_io.tellg();
        m_io.seekg(cur_pos,std::ios_base::beg);
    }
    else{
        if(m_mode & std::ios_base::out){
            m_io.seekp(0,std::ios_base::end);
            max_pos = m_io.tellp();
            m_io.seekp(cur_pos,std::ios_base::beg);
        }
        else{
            THROW(util::exception,"file mode was not supported for seeking");
        }
    }

    switch (whence) {
        case std::ios_base::beg:
            next_pos = off;
            break;
        case std::ios_base::cur:
            next_pos = off + cur_pos;
            break;
        case std::ios_base::end:
            next_pos = off + max_pos;
        break;
        default:
            next_pos = 0;
    }

    if(next_pos < 0)
        THROW(util::exception,"input seek was out of lower range; seek incomplete");
    if(next_pos > max_pos)
        THROW(util::exception,"input seek was out of upper range; seek incomplete");

    if(m_mode & std::ios_base::in){
        m_io.seekg(next_pos,std::ios_base::beg);
    }
    else{
        m_io.seekg(next_pos,std::ios_base::beg);
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
