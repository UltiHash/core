#include "temp_file.h"

#include <util/exception.h>

#include <unistd.h>
#include <fstream>

namespace uh::io
{

namespace
{

// ---------------------------------------------------------------------

const std::string FILENAME_TEMPLATE = "tempfile-XXXXXX";

// ---------------------------------------------------------------------

std::filesystem::path open_temp_file(std::filesystem::path& templ)
{
    auto path = (templ / FILENAME_TEMPLATE).string();

    int fd = mkstemp(path.data());
    if (fd == -1)
    {
        THROW_FROM_ERRNO();
    }

    close(fd);

    return path;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_file::temp_file(std::filesystem::path directory, std::ios_base::openmode mode)
    : file(open_temp_file(directory), mode),
      m_remove(true)
{
}

// ---------------------------------------------------------------------

temp_file::~temp_file()
{
    if (m_remove)
    {
        std::filesystem::remove(path());
    }
}

// ---------------------------------------------------------------------

void temp_file::release_to(const std::filesystem::path& path)
{
    if (this->path() == path)
    {
        m_remove = false;
        return;
    }

    if (::link(this->path().c_str(), path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }
}

// ---------------------------------------------------------------------

void temp_file::rename(const std::filesystem::path& path)
{
    if (::rename(this->path().c_str(), path.c_str()) == -1)
    {
        THROW_FROM_ERRNO();
    }
}

// ---------------------------------------------------------------------

} // namespace uh::io
