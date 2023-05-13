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
     * Create a temporary file in the given directory. The directory must exist.
     *
     * @throw the directory does not exist
     */
    explicit temp_file(const std::filesystem::path &directory);

    /**
     *
     * @param directory create temporary file here
     * @param mode work at some temporary file in another mode
     */
    explicit temp_file(const std::filesystem::path &directory, const std::string& mode);

    /**
     * Remove the temporary file.
     */
    ~temp_file() override;

    /**
     * Rename/Move the file to `path`, make it a permanent file
     *
     * @throw a file with the given name already exists
     */
    void release_to(const std::filesystem::path& path);

    /**
     * Rename the file to `path` overwriting already existing files.
     */
    void rename(const std::filesystem::path& path);

    /**
     *
     * @param at_directory is the directory where a temp file should be created
     * @return the path that can be converted into a valid temp file
     */
    static std::filesystem::path generate_valid_temp_path(const std::filesystem::path& at_directory);

private:
    bool m_remove{};

    /**
     * after creating a file object we need to set it up to match the needs of a temp file
     *
     * @param directory directory where the file should be created within
     */
    void temp_file_constructor(const std::filesystem::path &directory);
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
