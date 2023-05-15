#ifndef IO_FILE_H
#define IO_FILE_H

#include <io/seekable_device.h>
#include <util/exception.h>

#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/categories.hpp>

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <utility>


namespace uh::io
{

// ---------------------------------------------------------------------

class file : public seekable_device
{
public:
    explicit file(const std::filesystem::path& path);
    file(const std::filesystem::path& path, std::string mode);

    ~file() override;

    /**
     * writes to file and sets file invalid
     *
     * @param buffer input
     * @return size written
     */
    std::streamsize write(std::span<const char> buffer) override;

    /**
     * read to file and sets file invalid
     *
     * @param buffer input
     * @return size written
     */
    std::streamsize read(std::span<char> buffer) override;
    [[nodiscard]] bool valid() const override;
    void seek(off64_t pos) override;
    void seek(off64_t off, int whence) override;
    [[nodiscard]] std::size_t seekable_size() const override;
    void close();
    void open();
    void delete_file();

    /**
     * resets still open file and seeks to beginning of file
     */
    virtual void reset_file_state();

    /**
     * Return the path of the file
     */
    [[nodiscard]] const std::filesystem::path& path();

protected:
    std::filesystem::path m_path;
    std::string m_mode{};
    FILE* m_fp;

private:
    bool is_open = false;
    static void has_parent_path(const std::filesystem::path &path);
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
