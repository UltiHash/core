#include "temp_file.h"

#include <util/exception.h>

#include <unistd.h>


namespace uh::io
{

namespace
{

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
      m_path(),
      m_remove(true)
{
    if (!std::filesystem::exists(directory))
    {
        THROW(util::exception, "parent of temporary file does not exist");
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
    }

    if (m_remove)
    {
        unlink(m_path.c_str());
    }
}

// ---------------------------------------------------------------------

std::streamsize temp_file::write(std::span<const char> buffer)
{
    std::streamsize rv = 0;

    std::size_t n = buffer.size();
    const char* s = buffer.data();
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

std::streamsize temp_file::read(std::span<char> buffer)
{
    auto rv = ::read(m_fd, buffer.data(), buffer.size());

    if (rv == -1)
    {
        THROW_FROM_ERRNO();
    }

    return rv;
}

// ---------------------------------------------------------------------

bool temp_file::valid() const
{
    return m_fd != -1;
}

// ---------------------------------------------------------------------

void temp_file::release_to(const std::filesystem::path& path)
{
    if (m_path == path)
    {
        m_remove = false;
        return;
    }

    if (::link(m_path.c_str(), path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }
}

// ---------------------------------------------------------------------

void temp_file::rename(const std::filesystem::path& path)
{
    if (::rename(m_path.c_str(), path.c_str()) == -1)
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

const std::string temp_file::FILENAME_TEMPLATE = "tempfile-XXXXXX";

// ---------------------------------------------------------------------

} // namespace uh::io
