#ifndef UH_IO_TEMPFILE_H
#define UH_IO_TEMPFILE_H

#include <io/file.h>

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
class temp_file : public io::file
{
public:
    /**
     * Create a temporary file in the given directory. The directory must exist. If the input is
     * not a directory, but an existing file, it is treated as such file
     *
     * @throw the directory does not exist
     */
    explicit temp_file(std::filesystem::path directory);
    explicit temp_file(std::filesystem::path directory,std::ios_base::openmode mode);

    /**
     * Remove the temporary file.
     */
    ~temp_file() override;

    /**
     * Rename the file to `path` and make it a permanent file.
     *
     * @throw a file with the given name already exists
     */
    void release_to(const std::filesystem::path& path);

    /**
     * Rename the file to `path` overwriting already existing files.
     */
    void rename(const std::filesystem::path& path);

private:
    bool m_remove;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
