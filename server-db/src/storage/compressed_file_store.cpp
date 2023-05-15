#include "compressed_file_store.h"

#include <util/exception.h>
#include <logging/logging_boost.h>
#include <io/count.h>
#include <io/file.h>
#include <compression/compression.h>

#include <arpa/inet.h>


namespace uh::dbn::storage
{

namespace
{

// ---------------------------------------------------------------------

void write_comp_type(io::device& out, comp::type t)
{
    uint32_t nl_id = htonl(to_uint(t));

    if (out.write({ reinterpret_cast<char*>(&nl_id), sizeof(nl_id) }) < sizeof(nl_id))
    {
        THROW(util::exception, "failure writing compression header");
    }
}

// ---------------------------------------------------------------------

comp::type read_comp_type(io::device& in)
{
    uint32_t nl_id;
    if (in.read({ reinterpret_cast<char*>(&nl_id), sizeof(nl_id) }) < sizeof(nl_id))
    {
        THROW(util::exception, "failure reading compression header");
    }

    return comp::from_uint(ntohl(nl_id));
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> open_reader(const std::filesystem::path& path)
{
    auto in = std::make_unique<io::file>(path);

    comp::type type = read_comp_type(*in);
    return comp::create(std::move(in), type);
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

compression_worker::compression_worker(compressed_file_store& store,
                                       storage_metrics& metrics)
    : m_store(store),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

void compression_worker::operator()(const std::filesystem::path& path, comp::type type)
{
    std::streamsize savings = 0;
    try
    {
        m_store.start(path);
        auto in = open_reader(path);
        io::count count_in(*in);

        auto temp = io::temp_file(path.parent_path());
        io::count count_out(temp);

        write_comp_type(count_out, type);

        {
            auto out = comp::create(count_out, type);
            copy(count_in, *out);
        }

        temp.rename(path);

        savings = count_in.read() - count_out.written();
    }
    catch (...)
    {
        m_store.finish(path, savings);
        throw;
    }

    m_store.finish(path, savings);
}

// ---------------------------------------------------------------------

compressed_file_store::compressed_file_store(
    const compressed_file_store_config& config,
    storage_metrics& metrics,
    std::function<void(std::streamsize)> report_savings)
    : m_metrics(metrics),
      m_type(config.compression),
      m_active(0),
      m_report_savings(std::move(report_savings)),
      m_worker(config.threads, compression_worker(*this, m_metrics))
{
}

// ---------------------------------------------------------------------

std::unique_ptr<io::temp_file> compressed_file_store::temp_file(const std::filesystem::path& path)
{
    auto rv = std::make_unique<io::temp_file>(path);

    write_comp_type(*rv, comp::type::none);

    return rv;
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> compressed_file_store::open(const std::filesystem::path& path)
{
    return open_reader(path);
}

// ---------------------------------------------------------------------

void compressed_file_store::compress(const std::filesystem::path& path)
{
    std::unique_lock<std::mutex> lock(m_comp_mutex);
    auto [it, success] = m_compressing.emplace(path);
    if (!success)
    {
        return;
    }

    try
    {
        m_worker.push(path, m_type);
        m_metrics.comp_scheduled().Set(m_compressing.size());
    }
    catch (...)
    {
        m_compressing.erase(path);
        throw;
    }

    INFO << "scheduled compression of " << path;
}

// ---------------------------------------------------------------------

void compressed_file_store::start(const std::filesystem::path& path)
{
    ++m_active;
    m_metrics.comp_running().Set(m_active);
    INFO << "starting compression of " << path;
}

// ---------------------------------------------------------------------

void compressed_file_store::finish(const std::filesystem::path& path,
                                   std::streamsize savings)
{
    std::unique_lock<std::mutex> lock(m_comp_mutex);

    m_compressing.erase(path);
    m_metrics.comp_scheduled().Set(m_compressing.size());

    --m_active;
    m_metrics.comp_running().Set(m_active);
    m_report_savings(savings);

    INFO << "finished compression of " << path;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
