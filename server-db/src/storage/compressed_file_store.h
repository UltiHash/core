#ifndef SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H
#define SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H

#include <util/job_queue.h>
#include <io/device.h>
#include <io/temp_file.h>
#include <compression/type.h>
#include <persistence/scheduled_compressions_persistence.h>
#include <metrics/storage_metrics.h>

#include <filesystem>
#include <memory>
#include <set>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

class compressed_file_store;

// ---------------------------------------------------------------------

using uh::dbn::metrics::storage_metrics;

// ---------------------------------------------------------------------

class compression_worker
{
public:
    compression_worker(compressed_file_store& store, storage_metrics& metrics);

    void operator()(const std::filesystem::path& path, comp::type type);

private:
    compressed_file_store& m_store;
    storage_metrics& m_metrics;
};

// ---------------------------------------------------------------------

class compressed_file_store
{
public:
    /**
     * handle paths relative to base path
     */
    compressed_file_store(
        const compressed_file_store_config& config,
        storage_metrics& metrics,
        persistence::scheduled_compressions_persistence& scheduled_compressions,
        std::function<void(std::streamsize)> report_savings = [](std::streamsize){});

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
     * Stops the underlying queue.
     */
    void stop();

private:
    friend class compression_worker;

    void start(const std::filesystem::path& path);
    void finish(const std::filesystem::path& path, std::streamsize savings);

    storage_metrics& m_metrics;

    std::mutex m_comp_mutex;
    persistence::scheduled_compressions_persistence& m_scheduled_compressions;

    comp::type m_type;
    std::atomic<unsigned> m_active;
    std::function<void(std::streamsize)> m_report_savings;

    util::job_queue<void, std::filesystem::path, comp::type> m_worker;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
