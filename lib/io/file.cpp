
#include <util/exception.h>
#include "temp_file.h"
#include "file.h"

#include <utility>

namespace uh::io
{

const std::filesystem::path &file::path() {
    return m_path;
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path& path)
    : m_path(path),m_mode("a+")
{
    m_fp = nullptr;
    has_parent_path(path);

    if(!std::filesystem::is_directory(path)){
        open();
    }
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::string openmode)
    : m_path(path),m_mode(std::move(openmode))
{
    m_fp = nullptr;
    has_parent_path(path);

    if(!std::filesystem::exists(path) ^ !std::filesystem::is_directory(path)){
        open();
    }
}

// ---------------------------------------------------------------------

file::file(const std::filesystem::path &path, std::ios_base::openmode mode) {
    std::string c_mode;

    if(mode & std::ios_base::in)c_mode += "r";
    else if(mode & std::ios_base::out)c_mode += "w+";
    else if(mode & std::ios_base::app)c_mode += "a";
    else if(mode & std::ios_base::trunc)c_mode += "w";

    if(mode & std::ios_base::binary)c_mode += "b";

    m_mode = c_mode;

    m_fp = nullptr;
    has_parent_path(path);

    if(!std::filesystem::exists(path) ^ !std::filesystem::is_directory(path)){
        open();
    }
    if(mode & std::ios_base::ate)fseek(m_fp,0,SEEK_END);
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
    auto read_size = fread(buffer.data(), 1, buffer.size(),  m_fp);
    return read_size;
}

// ---------------------------------------------------------------------

bool file::valid() const
{
    return m_fp != nullptr && ftell(m_fp) < seekable_size() ;
}

// ---------------------------------------------------------------------

void file::seek(off64_t pos) {
    fseek(m_fp,pos,SEEK_SET);
}

// ---------------------------------------------------------------------

void file::seek(off64_t off, int whence) {
    fseek(m_fp,off,whence);
}

// ---------------------------------------------------------------------

std::size_t file::seekable_size() const {
    auto last_pos = ftell(m_fp);

    fseek(m_fp, 0L, SEEK_END);
    auto seek_size = ftell(m_fp);

    fseek(m_fp,last_pos,SEEK_SET);

    return seek_size;
}

// ---------------------------------------------------------------------

void file::open() {
    if(m_fp == nullptr)
        m_fp = fopen64(m_path.c_str(),m_mode.c_str());
}

// ---------------------------------------------------------------------

void file::close() {
    if(valid()){
        fflush(m_fp);
        fclose(m_fp);
        m_fp = nullptr;
    }
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

file::~file() {
    close();
}

// ---------------------------------------------------------------------

} // namespace uh::io
