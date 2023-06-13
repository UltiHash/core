#ifndef CORE_FUSE_CONTEXT_H
#define CORE_FUSE_CONTEXT_H

#include <protocol/client_pool.h>
#include <uhv/file.h>
#include "thread_safe_type.h"

#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>


namespace uh::fuse
{

// ---------------------------------------------------------------------

struct context
{
    context(const std::filesystem::path& uhv,
                    const std::string& hostname,
                    uint16_t port,
                    unsigned connections);

    unsigned long fd = 0;

    uhv::file& uhv();
    void update_uhv();

    protocol::client_pool::handle get_client();


    auto metadata_map()
    {
        return m_metadata_map.get();
    }

    auto open_files()
    {
        return m_open_files.get();
    }

    auto subdirectory_counts()
    {
        return m_subdirectory_counts.get();
    }

private:
    void init_metadata_map();

    boost::asio::io_context m_io;
    uhv::file m_uhv;
    protocol::client_pool m_pool;
    ts_container m_metadata_map;
    thread_safe_type<std::unordered_map<std::filesystem::path, unsigned long>> m_subdirectory_counts;
    thread_safe_type<std::unordered_map<unsigned long, ts_meta_data&>> m_open_files;
};

// ---------------------------------------------------------------------

} // namespace uh::fuse

#endif
