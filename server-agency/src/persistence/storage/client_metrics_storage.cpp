#include "client_metrics_storage.h"
#include <io/file.h>
#include <persistence/options.h>

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

client_metrics::client_metrics(const persistence_config& config) :
    m_target_path(config.persistence_path / std::filesystem::path("client_metrics/uhv_metrics.ua"))
{
}

// ---------------------------------------------------------------------

void client_metrics::start()
{
    if (std::filesystem::exists(m_target_path))
    {
        retrieve();
    }
    else
    {
        std::filesystem::create_directory(m_target_path.parent_path());
    }

    INFO << "client metrics persistence on: " << m_target_path;
}

// ---------------------------------------------------------------------

void client_metrics::add(const uh::protocol::client_statistics::request& req)
{
    m_id_to_size.insert_or_assign(std::string(req.uhv_id.begin(), req.uhv_id.end()), req.integrated_size);
}

// ---------------------------------------------------------------------

void client_metrics::flush()
{
    io::file metrics_file(m_target_path, std::ios::out | std::ios::trunc | std::ios::binary);

    uh::serialization::buffered_serializer serializer(metrics_file);
    serializer.write(m_id_to_size.size());

    for (const auto& pair : m_id_to_size)
    {
        serializer.write(pair.first);
        serializer.write(pair.second);
    }

    serializer.sync();
}

// ---------------------------------------------------------------------

void client_metrics::retrieve()
{
    io::file metrics_file(m_target_path);
    uh::serialization::sl_deserializer deserializer(metrics_file);

    auto count = deserializer.read<std::size_t>();

    while (count--)
    {
        auto uhv_id = deserializer.read<std::string>();
        auto integrated_size = deserializer.read<std::uint64_t>();
        m_id_to_size.insert({uhv_id, integrated_size});
    }
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
