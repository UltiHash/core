#include "protocol.h"

#include <logging/logging_boost.h>
#include <io/group_generator.h>
#include <protocol/exception.h>

#include <config.hpp>
#include <storage/backends/hierarchical_storage.h>
#include <util/structured_queries.h>
#include <numeric>


using namespace uh::protocol;

namespace uh::dbn::server
{

// ---------------------------------------------------------------------

protocol::protocol(storage::backend& storage, const uh::net::server_info& serv_info)
    : m_storage(storage),
      m_serv_info(serv_info)
{
}

// ---------------------------------------------------------------------

server_information protocol::on_hello(const std::string& client_version)
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

uh::protocol::write_chunks::response protocol::on_write_chunks(const write_chunks::request &req)
{

        size_t offset = 0;
        uh::protocol::write_chunks::response res;
        res.effective_size = 0;

        for (const auto size: req.chunk_sizes) {
            const auto result = m_storage.write_block({req.data.data() + offset, size});
            res.effective_size += result.first;
            res.hashes.insert(res.hashes.end(), result.second.cbegin(), result.second.cend());
            offset += size;
        }
        return res;
}

// ---------------------------------------------------------------------

uh::protocol::read_chunks::response protocol::on_read_chunks(const read_chunks::request& req)
{
    auto generator = std::make_unique<io::group_generator>();

    uh::protocol::read_chunks::response resp;
    for (size_t i = 0; i < req.hashes.size(); i += 64)
    {
        auto dev = m_storage.read_block({const_cast <char*> (req.hashes.data()) + i, 64 });

        auto size = dev->size();
        generator->append(std::move(dev));

        resp.chunk_sizes.emplace_back(size);
    }

    resp.data = std::move(generator);

    return resp;
}

uh::protocol::write_key_value::response protocol::on_write_kv(const write_key_value::request &request) {

    uh::util::structured_queries <write_key_value::request> wqs (request);

    write_key_value::response resp {.effective_sizes = util::ospan<uint32_t> (std::get <0> (request.key_sizes).size)};

    int i = 0;
    for (auto wq = wqs.next(); wq != nullptr; wq = wqs.next()) {
        resp.effective_sizes.data [i++] = m_storage.write_key_value(wq->key, wq->value);
    }

    return resp;
}

// ---------------------------------------------------------------------

uh::protocol::read_key_value::response protocol::on_read_kv(const read_key_value::request &request) {

    // TODO maybe a lock here?

    uh::util::structured_queries <read_key_value::request> queries (request);
    auto generator = std::make_unique<io::group_generator>();
    uh::protocol::read_key_value::response resp {.key_sizes = std::vector <uint16_t>{}, .value_sizes = std::vector <uint32_t> {}};

    for (auto query = queries.next(); query != nullptr; query = queries.next()) {

        std::span <std::string_view> labels {query->labels.data.get(), query->labels.size};

        if (!query->single_key.empty()) {
            generator->append(m_storage.read_value(query->single_key, labels));
        }
        else if (!query->start_key.empty() or !query->end_key.empty()) {

            auto key_values = m_storage.fetch_query(query->start_key, query->end_key, labels);

            for (auto& key_value: key_values) {
                std::get <1> (resp.key_sizes).emplace_back(key_value.key->size());
                std::get <1> (resp.value_sizes).emplace_back(key_value.value->size());
                generator->append(std::move(key_value.key));
                generator->append(std::move(key_value.value));
            }
        }
    }

    resp.data = std::move(generator);
    return resp;

}

// ---------------------------------------------------------------------

} // namespace uh::dbn::server
