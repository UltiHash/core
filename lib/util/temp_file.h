#ifndef UH_UTIL_TEMPFILE_H
#define UH_UTIL_TEMPFILE_H

#include <filesystem>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/positioning.hpp>

namespace uh::util
{

// ---------------------------------------------------------------------

using boost::iostreams::stream_offset;

// ---------------------------------------------------------------------

/**
 * Temporary file with self-cleanup. The file implements Boost's seekable device
 * interface.
 */
class temp_file
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
     * Rename the file to `path` and make it a permanent file.
     *
     * @throw a file with the given name already exists
     */
    void release_to(const std::filesystem::path& path);

    /**
     * Return the path of the temporary file.
     */
    const std::filesystem::path& path() const;

    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    std::streamsize write(const char* s, std::streamsize n);
    std::streamsize read(char* s, std::streamsize n);
    std::streampos seek(stream_offset off, std::ios_base::seekdir way);

    const static std::string FILENAME_TEMPLATE;

private:
    int m_fd;
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
