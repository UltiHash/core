#ifndef SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H
#define SERVER_DB_STORAGE_COMPRESSED_FILE_WRITER_H

#include <util/job_queue.h>
#include <util/exception.h>
#include <io/count.h>
#include <io/file.h>
#include <io/device.h>
#include <io/temp_file.h>
#include <io/buffer.h>
#include <compression/compression.h>
#include <logging/logging_boost.h>

#include <arpa/inet.h>

#include <metrics/storage_metrics.h>

#include <filesystem>
#include <memory>
#include <set>


namespace uh::dbn::storage
{

// ---------------------------------------------------------------------

void write_comp_type(io::device& out, comp::type t);

// ---------------------------------------------------------------------

comp::type read_comp_type(io::device& in);

// ---------------------------------------------------------------------

std::unique_ptr<io::device> open_reader(const std::filesystem::path& path);

// ---------------------------------------------------------------------

struct compressed_file_store_config
{
    static constexpr unsigned DEFAULT_THREADS = 5u;

    /**
     * Number of threads for background compression.
     */
    unsigned threads = DEFAULT_THREADS;

    /**
     * Default compression applied to files.
     */
    comp::type compression = comp::type::none;
};

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
        std::function<void(std::streamsize)> report_savings = [](std::streamsize){} );

    /**
     * Open a temporary file for writing
     */
    static std::unique_ptr<io::temp_file> temp_file(const std::filesystem::path& path);

    /**
     * Open a file for reading.
     */
    static std::unique_ptr<io::device> open(const std::filesystem::path& path);

    /**
     * Start compressing file transparently in the background. Does not throw.
     */
    void compress(const std::filesystem::path& path);

private:
    friend class compression_worker;

    void start(const std::filesystem::path& path);
    void finish(const std::filesystem::path& path, std::streamsize savings);

    storage_metrics& m_metrics;

    std::mutex m_comp_mutex;
    std::set<std::filesystem::path> m_compressing;

    comp::type m_type;
    std::atomic<unsigned> m_active;
    std::function<void(std::streamsize)> m_report_savings;

    util::job_queue<void, std::filesystem::path, comp::type> m_worker;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage

#endif
