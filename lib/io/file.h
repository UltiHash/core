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
    explicit file(const std::filesystem::path& path,
                  std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;
    bool valid() const override;

    void seek(std::streamoff off, std::ios_base::seekdir whence) override;

    void reopen();

    void close();

    /**
     * @return the path of the file.
     */
    [[nodiscard]] std::filesystem::path path();

    std::ios_base::openmode mode();

    std::size_t size();

private:
    std::fstream m_io;
    std::ios_base::openmode m_mode;
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
