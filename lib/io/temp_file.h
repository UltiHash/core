#ifndef UH_IO_TEMPFILE_H
#define UH_IO_TEMPFILE_H

#include <io/device.h>

#include <filesystem>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/positioning.hpp>


namespace uh::io
{

// ---------------------------------------------------------------------

using boost::iostreams::stream_offset;

// ---------------------------------------------------------------------

/**
 * Temporary file with self-cleanup. The file implements Boost's seekable device
 * interface.
 */
class temp_file : public io::device
{
public:
    /**
     * Create a temporary file in the given directory. The directory must exist.
     *
     * @throw the directory does not exist
     */
    temp_file(const std::filesystem::path& directory);

    /**
     * Remove the temporary file.
     */
    ~temp_file();

    /**
     * Write the contents of the span and return the number of bytes written.
     *
     * @throw writing fails for any reason
     */
    virtual std::streamsize write(std::span<const char> buffer) override;

    /**
     * Read from the device and store it in the buffer. Return the number of
     * bytes read.
     *
     * @throw error while reading
     */
    virtual std::streamsize read(std::span<char> buffer) override;

    /**
     * Return whether this device still can be used.
     */
    virtual bool valid() const override;

    /**
     * Rename the file to `path` and make it a permanent file.
     *
     * @throw a file with the given name already exists
     */
    void release_to(const std::filesystem::path& path);

    /**
     * Return the path of the temporary file.
     */
    const std::filesystem::path& path() const;

    std::streampos seek(stream_offset off, std::ios_base::seekdir way);

    const static std::string FILENAME_TEMPLATE;

private:
    int m_fd;
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
