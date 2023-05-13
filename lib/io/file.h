#ifndef IO_FILE_H
#define IO_FILE_H

#include <io/file.h>
#include <io/seekable_device.h>
#include <util/exception.h>

#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/categories.hpp>

#include <filesystem>
#include <fstream>
#include <cstdio>


namespace uh::io
{

// ---------------------------------------------------------------------

class file : public seekable_device
{
public:
    explicit file(const std::filesystem::path& path);
    file(const std::filesystem::path& path, std::string  mode);

    ~file() override;

    /**
     * writes to file and closes it to invalidate
     *
     * @param buffer input
     * @return size written
     */
    std::streamsize write(std::span<const char> buffer) override;
    /**
     * read to file and closes it to invalidate
     *
     * @param buffer input
     * @return size written
     */
    std::streamsize read(std::span<char> buffer) override;
    [[nodiscard]] bool valid() const override;
    void seek(off64_t pos) override;
    void seek(off64_t off, int whence) override;
    std::size_t seekable_size() override;
    void close();
    void open();
    void delete_file();

    /**
     * Return the path of the file
     */
    [[nodiscard]] const std::filesystem::path& path() const;

protected:
    std::filesystem::path m_path;
    std::string m_mode{};
    FILE* m_fp;

private:
    static void has_parent_path(const std::filesystem::path &path);
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
