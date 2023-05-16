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

std::tuple<int , std::filesystem::path> open_temp_file(std::filesystem::path& templ)
{
    auto path = templ.string();

    int fd = mkstemp(path.data());
    if (fd == -1)
    {
        THROW_FROM_ERRNO();
    }

    templ = std::filesystem::path(path);

    return {fd, templ};
}

std::filesystem::path convert_to_stream_path(std::filesystem::path &fd_path){
    if (!std::filesystem::exists(fd_path))
    {
        THROW(util::exception, "parent of temporary file does not exist");
    }

    if(std::filesystem::is_directory(fd_path))
    {
        fd_path = fd_path / FILENAME_TEMPLATE;
        auto [fd, path] = open_temp_file(fd_path);
        close(fd);

        fd_path = path;
    }
    return fd_path;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_file::temp_file(std::filesystem::path directory)
    : file(convert_to_stream_path(directory),std::ios_base::app), m_remove(true)
{}

// ---------------------------------------------------------------------

temp_file::temp_file(std::filesystem::path directory,std::ios_base::openmode mode)
        : file(convert_to_stream_path(directory), mode), m_remove(true)
{}

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
