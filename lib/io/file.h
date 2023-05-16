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
    /**
     * files are in read mode on default, any other behaviour needs to be manually defined
     * on its mode
     *
     * @param path input path to work on
     */
    explicit file(const std::filesystem::path& path);
    file(const std::filesystem::path& path, std::ios_base::openmode mode);

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;
    bool valid() const override;

    void seek (std::streamoff off, const std::ios_base::seekdir whence) override;

    /**
     * Return the path of the temporary file.
     */
    [[nodiscard]] std::filesystem::path path();

private:
    std::fstream m_io;
    std::filesystem::path m_path;
    std::ios_base::openmode m_mode;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
