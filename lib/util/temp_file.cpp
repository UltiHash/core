#include "temp_file.h"

#include <util/exception.h>

#include <unistd.h>


namespace uh::util
{

namespace
{

// ---------------------------------------------------------------------

int seekdir_to_int(std::ios_base::seekdir way)
{
    switch (way)
    {
        case std::ios_base::beg: return SEEK_SET;
        case std::ios_base::cur: return SEEK_CUR;
        case std::ios_base::end: return SEEK_END;
    }

    THROW(exception, "unsupported seekdir value");
}

// ---------------------------------------------------------------------

std::pair<int, std::filesystem::path> open_temp_file(const std::filesystem::path& templ)
{
    auto path = templ.string();

    int fd = mkstemp(path.data());
    if (fd == -1)
    {
        THROW_FROM_ERRNO();
    }

    return std::make_pair(fd, std::filesystem::path(path));
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path& directory)
    : m_fd(-1),
      m_path()
{
    if (!std::filesystem::exists(directory))
    {
        THROW(exception, "parent of temporary file does not exist");
    }

    auto [fd, path] = open_temp_file(directory / FILENAME_TEMPLATE);
    m_fd = fd;
    m_path = path;
}

// ---------------------------------------------------------------------

temp_file::~temp_file()
{
    if (m_fd != -1)
    {
        close(m_fd);
        unlink(m_path.c_str());
    }
}

// ---------------------------------------------------------------------

void temp_file::release_to(const std::filesystem::path& path)
{
    if (link(m_path.c_str(), path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }
}

// ---------------------------------------------------------------------

const std::filesystem::path& temp_file::path() const
{
    return m_path;
}

// ---------------------------------------------------------------------

std::streamsize temp_file::write(const char* s, std::streamsize n)
{
    std::streamsize rv = 0;

    while (n > 0)
    {
        auto written = ::write(m_fd, s, n);

        if (written == -1)
        {
            THROW_FROM_ERRNO();
        }

        n -= written;
        rv += written;
        s += written;
    }

    return rv;
}

// ---------------------------------------------------------------------

std::streamsize temp_file::read(char* s, std::streamsize n)
{
    auto rv = ::read(m_fd, s, n);

    if (rv == -1)
    {
        THROW_FROM_ERRNO();
    }

    return rv;
}

// ---------------------------------------------------------------------

std::streampos temp_file::seek(stream_offset off, std::ios_base::seekdir way)
{
    int whence = seekdir_to_int(way);

    auto rv = lseek(m_fd, off, whence);
    if (rv == -1)
    {
        THROW_FROM_ERRNO();
    }

    return rv;
}

// ---------------------------------------------------------------------

const std::string temp_file::FILENAME_TEMPLATE = "tempfile-XXXXXX";

// ---------------------------------------------------------------------

} // namespace uh::util
