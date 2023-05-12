#include "file.h"

#include <utility>


namespace uh::io
{

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path)
{
    has_parent_path(path);

    if(!std::filesystem::exists(path) ^ !std::filesystem::is_directory(path)){
        open();
    }
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::string openmode)
    : m_path(path),m_mode(std::move(openmode))
{
    has_parent_path(path);

    if(!std::filesystem::exists(path) ^ !std::filesystem::is_directory(path)){
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
    auto written_size = fwrite(buffer.begin().base(), buffer.size(), 1, m_fp);
    return written_size;
}

// ---------------------------------------------------------------------

std::streamsize file::read(std::span<char> buffer)
{
    auto read_size = fread(buffer.data(), buffer.size(), 1, m_fp);
    return read_size;
}

// ---------------------------------------------------------------------

bool file::valid() const
{
    return m_fp != nullptr && !feof(m_fp);
}

void file::seek(off64_t pos) {
    fseek(m_fp,pos,SEEK_SET);
}

void file::seek(off64_t off, int whence) {
    fseek(m_fp,off,whence);
}

std::size_t file::seekable_size()
{
    auto last_pos = ftell(m_fp);

    fseek(m_fp, 0L, SEEK_END);
    auto seek_size = ftell(m_fp);

    fseek(m_fp,last_pos,SEEK_SET);

    return seek_size;
}

void file::open() {
    if(m_fp == nullptr)m_fp = fopen64(m_path.c_str(),m_mode.c_str());
}

void file::close() {
    fclose(m_fp);
}

file::~file() {
    close();
}

// ---------------------------------------------------------------------

} // namespace uh::io
