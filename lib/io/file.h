#ifndef IO_FILE_H
#define IO_FILE_H

#include <io/seekable_device.h>

#include <filesystem>
#include <fstream>


namespace uh::io
{

// ---------------------------------------------------------------------

class file : public seekable_device
{
public:
    explicit file(const std::filesystem::path& path);
    file(const std::filesystem::path& path, std::ios_base::openmode mode);

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;
    bool valid() const override;
    void seek(off64_t pos) override;
    void seek(off64_t off,std::ios_base::seekdir way) override;

private:
    std::fstream m_io;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
