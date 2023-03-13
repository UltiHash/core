#ifndef IO_FILE_H
#define IO_FILE_H

#include <io/device.h>

#include <filesystem>
#include <fstream>


namespace uh::io
{

// ---------------------------------------------------------------------

class file : public device
{
public:
    explicit file(const std::filesystem::path& path);
    file(const std::filesystem::path& path, std::ios_base::openmode mode);

    virtual std::streamsize write(std::span<const char> buffer) override;
    virtual std::streamsize read(std::span<char> buffer) override;
    virtual bool valid() const override;
private:
    std::fstream m_io;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
