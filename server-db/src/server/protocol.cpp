#include "protocol.h"

#include <config.hpp>
#include <logging/logging_boost.h>
#include <protocol/exception.h>
#include <storage/backends/hierarchical_storage.h>
#include <numeric>


using namespace uh::protocol;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

protocol::protocol(storage::backend &storage, const uh::net::server_info &serv_info)
    : m_storage(storage),
      m_serv_info(serv_info),
      m_read_buffer(uh::protocol::server::MAXIMUM_DATA_SIZE)
{
}

// ---------------------------------------------------------------------

server_information protocol::on_hello(const std::string &client_version)
{
    if (m_serv_info.server_busy())
    {
        THROW(server_busy, "server is busy, try again later");
    }

    INFO << "connection from client with version " << client_version;

    return {
            .version = PROJECT_VERSION,
            .protocol = 1,
    };
}

// ---------------------------------------------------------------------

std::size_t protocol::on_free_space()
{
    return m_storage.free_space();
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> protocol::on_allocate_chunk(std::size_t size)
{
    return m_storage.allocate(size);
}

// ---------------------------------------------------------------------

uh::protocol::write_chunks::response protocol::on_write_chunks(const write_chunks::request &req)
{
    auto alloc = m_storage.allocate_multi (req.data.size());
    auto &multi_alloc = dynamic_cast <uh::dbn::storage::hierarchical_storage::hierarchical_multi_block_allocation&> (*alloc);
    size_t offset = 0;
    uh::protocol::write_chunks::response res;
    res.effective_size = 0;

    for (const auto size: req.chunk_sizes) {
        multi_alloc.open_new_block(size);
        multi_alloc.device().write({req.data.data() + offset, size});
        auto block_md = multi_alloc.persist();
        res.hashes.insert(res.hashes.end (), block_md.hash.cbegin(), block_md.hash.cend());
        res.effective_size += block_md.effective_size;
        offset += size;
    }

    return res;
}

// ---------------------------------------------------------------------

uh::protocol::read_chunks::response protocol::on_read_chunks(const read_chunks::request &req)
{
    uh::protocol::read_chunks::response resp;
    ssize_t total_size = 0;
    for (size_t i = 0; i < req.hashes.size(); i+=64) {
        auto dev = m_storage.read_block({req.hashes.data() + i, 64});
        const auto size = dev->read({m_read_buffer.begin() + total_size, m_read_buffer.end()});
        resp.chunk_sizes.emplace_back(size);
        total_size += size;
    }
    resp.data.reserve(total_size);
    resp.data.insert(resp.data.cend(), m_read_buffer.begin(), m_read_buffer.begin() + total_size);
    return resp;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
