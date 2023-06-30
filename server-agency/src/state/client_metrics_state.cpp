#include "client_metrics_state.h"
#include <io/file.h>
#include <io/temp_file.h>
#include <logging/logging_boost.h>

namespace uh::an::state
{

// ---------------------------------------------------------------------

client_metrics_state::client_metrics_state(const storage_config& config) :
    m_target_path(config.an_metrics / std::filesystem::path("uhv_metrics.uhs"))
{
}

// ---------------------------------------------------------------------

void client_metrics_state::start()
{
    if (std::filesystem::exists(m_target_path))
        retrieve();

    INFO << "client metrics persistent on: " << m_target_path;
}

// ---------------------------------------------------------------------

void client_metrics_state::stop()
{
    flush();
}

// ---------------------------------------------------------------------

void client_metrics_state::add(const uh::protocol::client_statistics::request& req)
{
    m_id_to_size.insert_or_assign(std::string(req.uhv_id.begin(), req.uhv_id.end()), req.integrated_size);
    flush();
}

// ---------------------------------------------------------------------

const std::map<std::string, std::uint64_t>& client_metrics_state::id_to_size_map() const
{
    return m_id_to_size;
}

// ---------------------------------------------------------------------

void client_metrics_state::flush()
{
    io::temp_file metrics_file(m_target_path.parent_path());

    uh::serialization::buffered_serializer serializer(metrics_file);
    serializer.write(m_id_to_size.size());

    for (const auto& pair : m_id_to_size)
    {
        serializer.write(pair.first);
        serializer.write(pair.second);
    }

    serializer.sync();
    metrics_file.rename(m_target_path);
}

// ---------------------------------------------------------------------

void client_metrics_state::retrieve()
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

} // namespace uh::an::state
