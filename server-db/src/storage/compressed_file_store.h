#ifndef SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H
#define SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H

#include <util/job_queue.h>
#include <io/device.h>
#include <io/temp_file.h>
#include <compression/type.h>

#include <filesystem>
#include <memory>
#include <set>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

struct compressed_file_store_config
{
    /**
     * Base directory for compressed file storage.
     */
    std::filesystem::path base_direcory;

    /**
     * Number of threads for background compression.
     */
    unsigned threads;

    /**
     * Default compression applied to files.
     */
    comp::type compression = comp::type::none;
};

// ---------------------------------------------------------------------

class compressed_file_store
{
public:
    /**
     * handle paths relative to base path
     */
    compressed_file_store(const compressed_file_store_config& config);

    /**
     * Open a temporary file for writing
     */
    std::unique_ptr<io::temp_file> temp_file(const std::filesystem::path& path);

    /**
     * Open a file for reading.
     */
    std::unique_ptr<io::device> open(const std::filesystem::path& path);

    /**
     * Start compressing file transparently in the background. Does not throw.
     */
    void compress(const std::filesystem::path& path);

    /**
     * Finalize compression by removing the path from the internal compression
     * queue, allowing for recompression in the future.
     */
    void finish(const std::filesystem::path& path);

private:
    unsigned m_threads;
    util::job_queue<void, std::filesystem::path, comp::type, compressed_file_store*> m_worker;

    std::mutex m_comp_mutex;
    std::set<std::filesystem::path> m_compressing;
    comp::type m_type;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
