
#include "file.h"

namespace uh::io
{

const std::filesystem::path &file::path() {
    return m_path;
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path),m_mode("a+")
{
    has_parent_path(path);

    if(!std::filesystem::is_directory(path)){
        open();
    }
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::string openmode)
    : m_path(path),m_mode(std::move(openmode))
{
    has_parent_path(path);

    if(!std::filesystem::is_directory(path)){
        open();
    }
}

// ---------------------------------------------------------------------

void file::has_parent_path(const std::filesystem::path &path) {
    if((path.has_parent_path() && !std::filesystem::exists(path.parent_path()))||
    (!path.has_parent_path() && !std::filesystem::exists(path)))
        THROW(util::exception,"the directory where the file should have been opened or created to, did not exist");
}

// ---------------------------------------------------------------------

std::streamsize file::write(std::span<const char> buffer)
{
    auto written_size = fwrite(buffer.data(), 1, buffer.size(), m_fp);
    fflush(m_fp);
    return written_size;
}

// ---------------------------------------------------------------------

std::streamsize file::read(std::span<char> buffer)
{
    if(valid())return fread(buffer.data(), 1, buffer.size(),  m_fp);
    else return 0;
}

// ---------------------------------------------------------------------

bool file::valid() const
{
    return is_open && (
    std::find(m_mode.begin(),m_mode.end(),'w') != m_mode.end() ||
    std::find(m_mode.begin(),m_mode.end(),'a') != m_mode.end() ||
            ftell(m_fp) < seekable_size());
}

// ---------------------------------------------------------------------

void file::seek(std::streamoff pos) {
    fseek(m_fp,pos,SEEK_SET);
}

// ---------------------------------------------------------------------

void file::seek(std::streamoff off, int whence) {
    fseek(m_fp,off,whence);
}

// ---------------------------------------------------------------------

std::size_t file::seekable_size() const {
    auto last_pos = ftell(m_fp);

    fseek(m_fp, 0L, SEEK_END);
    auto file_size = ftell(m_fp);

    fseek(m_fp,last_pos,SEEK_SET);

    return file_size-last_pos;
}

// ---------------------------------------------------------------------

void file::open() {
    m_fp = fopen(m_path.c_str(),m_mode.c_str());
    is_open = true;
}

// ---------------------------------------------------------------------

void file::delete_file() {
    unlink(m_path.c_str());
}

// ---------------------------------------------------------------------

void file::reset_file_state() {
    seek(0);
}

// ---------------------------------------------------------------------

} // namespace uh::io
