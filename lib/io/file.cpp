#include "file.h"

#include "util/exception.h"

namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode)
    : m_path(path),m_mode(mode)
{

    m_io = std::fstream(path,mode);
    m_io.exceptions( std::ifstream::badbit | std::ifstream::failbit );

    if (!m_io.is_open())
        throw std::runtime_error("Could not open the file!");
    else{
        eof_count = std::filesystem::file_size(path);
    }

}

// ---------------------------------------------------------------------

std::streamsize file::write(std::span<const char> buffer)
{
    std::streamoff start_tell,end_tell;

    if(m_mode & std::ios_base::in){
        start_tell = m_io.tellg();
    }
    else{
        start_tell = m_io.tellp();
    }

    m_io.write(buffer.data(), buffer.size());

    if(m_mode & std::ios_base::in){
        end_tell = m_io.tellg();
    }
    else{
        end_tell = m_io.tellp();
    }

    eof_count = std::max(eof_count,end_tell-start_tell);

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
    std::streampos cur_pos;

    if(m_mode & std::ios_base::in){
        cur_pos = m_io.tellg();
    }
    else{
        cur_pos = m_io.tellp();
    }

    std::streampos next_pos;

    switch (whence) {
        case std::ios_base::beg:
            next_pos = off;
            break;
        case std::ios_base::cur:
            next_pos = off + cur_pos;
            break;
        case std::ios_base::end:
            next_pos = off + eof_count;
            break;
        default:
            next_pos = 0;
    }

    if(next_pos < 0)
        THROW(util::exception,"input seek was out of lower range; seek incomplete");
    if(next_pos > eof_count)
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

} // namespace uh::io
