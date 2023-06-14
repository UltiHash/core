#include "scheduled_compressions_persistence.h"
#include <logging/logging_boost.h>
#include <io/temp_file.h>
#include <io/file.h>
#include <serialization/serialization.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

scheduled_compressions_persistence::scheduled_compressions_persistence(const uh::dbn::storage::storage_config& config) :
    m_target_path(config.db_metrics / std::filesystem::path("scheduled_compressions.uhs"))
{
}

// ---------------------------------------------------------------------

scheduled_compressions_persistence::scheduled_compressions_persistence() :
        m_target_path("/tmp" / std::filesystem::path("scheduled_compressions.ua"))
{
}

// ---------------------------------------------------------------------

void scheduled_compressions_persistence::start()
{
    if (std::filesystem::exists(m_target_path))
        retrieve();

    INFO << "scheduling info for compression persistent on: " << m_target_path;
}

// ---------------------------------------------------------------------

std::pair<std::set<std::filesystem::path>::iterator, bool> scheduled_compressions_persistence::
                                                           insert(const std::filesystem::path& path)
{
    auto ret_value = m_scheduled.insert(path);
    flush();

    return ret_value;
}

// ---------------------------------------------------------------------

void scheduled_compressions_persistence::erase(const std::filesystem::path& path)
{
    m_scheduled.erase(path);
    flush();

}

// ---------------------------------------------------------------------

std::size_t scheduled_compressions_persistence::size() const
{
    return m_scheduled.size();
}

// ---------------------------------------------------------------------

bool scheduled_compressions_persistence::empty() const
{
    return m_scheduled.empty();
}

// ---------------------------------------------------------------------

const std::set<std::filesystem::path>& scheduled_compressions_persistence::set() const
{
    return m_scheduled;
}

// ---------------------------------------------------------------------

void scheduled_compressions_persistence::flush()
{
    io::temp_file scheduled_compressions(m_target_path.parent_path());

    uh::serialization::buffered_serializer serializer(scheduled_compressions);
    serializer.write(m_scheduled.size());

    for (const auto &path : m_scheduled)
    {
        serializer.write(path.string());
    }

    serializer.sync();
    scheduled_compressions.rename(m_target_path);
}

// ---------------------------------------------------------------------

void scheduled_compressions_persistence::retrieve()
{
    io::file scheduled_compressions(m_target_path);
    uh::serialization::sl_deserializer deserializer(scheduled_compressions);

    auto count = deserializer.read<std::size_t>();

    while (count--)
    {
        auto path= deserializer.read<std::string>();
        m_scheduled.insert(path);
    }
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::persistence
