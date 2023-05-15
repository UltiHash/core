#include "temp_file.h"

#include <util/exception.h>

#include <unistd.h>
#include <fstream>

namespace uh::io
{

namespace
{

// ---------------------------------------------------------------------

std::tuple<int , std::filesystem::path> open_temp_file(const std::filesystem::path& templ)
{
    auto path = templ.string();

    int fd = mkstemp(path.data());
    if (fd == -1)
    {
        THROW_FROM_ERRNO();
    }

    return {fd, std::filesystem::path(path)};
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path &directory)
    : file(directory), m_remove(true)
{
    if (!std::filesystem::exists(directory))
    {
        THROW(util::exception, "parent of temporary file does not exist");
    }

    if(std::filesystem::is_directory(directory)){
        auto [fd, path] = open_temp_file(directory / FILENAME_TEMPLATE);
        close(fd);
        this->m_io = std::fstream(path);
        m_path = path;
    }
}

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path &directory,std::ios_base::openmode mode)
        : file(directory,mode), m_remove(true)
{
    if (!std::filesystem::exists(directory))
    {
        THROW(util::exception, "parent of temporary file does not exist");
    }

    if(std::filesystem::is_directory(directory)){
        auto [fd, path] = open_temp_file(directory / FILENAME_TEMPLATE);
        close(fd);
        this->m_io = std::fstream(path,mode);
        m_path = path;
    }
}

// ---------------------------------------------------------------------

temp_file::~temp_file()
{
    if (m_io.is_open())
    {
        m_io.close();
    }

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

const std::string temp_file::FILENAME_TEMPLATE = "tempfile-XXXXXX";

// ---------------------------------------------------------------------

} // namespace uh::io
