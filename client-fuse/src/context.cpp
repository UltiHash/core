#include "context.h"

#include <protocol/client_factory.h>
#include <net/plain_socket.h>

#include "config.hpp"


namespace uh::fuse
{

namespace
{

// ---------------------------------------------------------------------

protocol::client_pool make_pool(boost::asio::io_context& io,
                                const std::string& hostname, uint16_t port,
                                unsigned connections)
{
    protocol::client_factory_config cf_config
    {
        .client_version = std::string(PROJECT_NAME " " PROJECT_VERSION)
    };

    auto factory = std::make_unique<protocol::client_factory>(
        std::make_unique<net::plain_socket_factory>(io, hostname, port),
        cf_config);

    return protocol::client_pool(std::move(factory), connections);
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

context::context(const std::filesystem::path& uhv,
                 const std::string& hostname,
                 uint16_t port,
                 unsigned connections)
    : m_io(),
      m_uhv(uhv),
      m_pool(make_pool(m_io, hostname, port, connections))
{
    init_metadata_map();
}

// ---------------------------------------------------------------------

uhv::file& context::uhv()
{
    return m_uhv;
}

// ---------------------------------------------------------------------

void context::update_uhv()
{
    auto container = metadata_map();
    auto& data = container();

    std::list<std::unique_ptr<uhv::f_meta_data>> md;
    for (auto& tsmd : data)
    {
        md.push_back(std::make_unique<uhv::f_meta_data>(tsmd.second.get()()));
    }

    m_uhv.serialize(md);
}

// ---------------------------------------------------------------------

protocol::client_pool::handle context::get_client()
{
    return m_pool.get();
}

// ---------------------------------------------------------------------

void context::init_metadata_map()
{
    auto metadata_list = m_uhv.deserialize();

    auto container_handle = m_metadata_map.get();
    auto& metadata_map = container_handle();

    auto subdirectories_handle = m_subdirectory_counts.get();
    auto& subdirectory_counts = subdirectories_handle();

    for (const auto& metadata : metadata_list)
    {
        if (metadata->f_type() == uhv::uh_file_type::directory)
        {
            auto parent = metadata->f_path().parent_path();
            if (const auto& it = subdirectory_counts.find(parent); it != subdirectory_counts.end())
            {
                it->second++;
            }
            else
            {
                subdirectory_counts.emplace(parent, 1);
            }

            subdirectory_counts.emplace(metadata->f_path(), 0);
        }

        metadata_map.emplace(metadata->f_path(), *metadata);
    }

    uhv::f_meta_data metadata;
    metadata.set_f_path("/");
    metadata.set_f_type(uhv::directory);
    metadata.set_f_size(0u);
    metadata_map.emplace("/", std::move(metadata));
    subdirectory_counts.emplace("/", 0);
}

// ---------------------------------------------------------------------

} // namespace uh::fuse
