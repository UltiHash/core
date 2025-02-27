#pragma once

#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/types/scoped_buffer.h"
#include "config.h"

#include <common/etcd/service_discovery/storage_service_get_handler.h>
#include <common/network/client.h>
#include <storage/interface.h>

namespace uh::cluster {

class client_factory {
public:
    client_factory(boost::asio::io_context& ioc, unsigned connections);

    std::shared_ptr<client> make_service(const std::string& hostname,
                                         uint16_t port, int);

private:
    boost::asio::io_context& m_ioc;
    unsigned m_connections;
};

struct global_data_view_config {
    std::size_t storage_service_connection_count = 16;
    std::size_t read_cache_capacity_l2 = 400000ul;
    std::size_t ec_data_shards = 1;
    std::size_t ec_parity_shards = 0;
};

class global_data_view : public sn::interface {

public:
    /**
     * @brief Constructs a global_data view.
     *
     * The global_data_view introduces the abstraction of a flat
     * address space that fragments can be written to and read from, hiding the
     * interaction with individual storage service instances.
     *
     * @param config A constant reference to an instance of
     * global_data_view_config, providing all tunable configuration parameters.
     * @param ioc A reference to an instance of boost::asio::io_context used for
     * spawning co-routines.
     * @param storage_maintainer A reference to an instance of
     * service maintainer used for service discovery.
     */
    global_data_view(
        boost::asio::io_context& ioc,
        service_maintainer<client, client_factory, STORAGE_SERVICE>&
            storage_maintainer);

    ~global_data_view();

    /**
     * @brief Sends write request to a storage service instance, does not
     * guarantee persistence.
     *
     * @param ctx traces context
     * @param data A constant reference to a std::string_view holding the data
     * to be written.
     * @return An #address the data has been written to.
     */
    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets);

    /**
     * @brief reads the data starting from pointer, up to the given size.
     * It is allowed to return data that is smaller than the requested size if
     * there is no more data left in the data store file.
     *
     * @param ctx traces context
     * @param pointer A constant reference to a uint128_t, specifying the
     * location of the size
     * @param size A size_t specifying the size of the fragment.
     * @return
     */
    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer);

    /**
     * @brief registers a reference to a storage region to claim co-ownership
     * of data.
     *
     * In the sense of optimistic concurrency control, this call
     * tries to increase the reference count of a storage region. If due to
     * a data race, data in the specified region has already been deleted,
     * an address with the affected regions is returned to enable upstream
     * handling of the issue.
     *
     * @param ctx traces context
     * @param addr The address specifying the storage regions to be referenced.
     * @return An address specifying all storage regions within #addr that have
     *
     * already been deleted and therefore can no longer be referenced.
     */
    [[nodiscard]] coro<address> link(context& ctx, const address& addr);

    /**
     * @brief un-registers a reference to a storage region to release
     * co-ownership of data.
     *
     * @param ctx traces context
     * @param addr The address specifying the storage regions to be
     * un-referenced.
     * @return number of bytes freed in response to removing references.
     * In case of an error, std::numeric_limits<std::size_t>::max() is returned.
     */
    coro<std::size_t> unlink(context& ctx, const address& addr);

    /**
     * @brief Computes used space across all available storage service
     * instances.
     * @param ctx open telemetry context
     * @return The used space across all available storage service instances.
     */
    coro<std::size_t> get_used_space(context& ctx);

private:
    boost::asio::io_context& m_io_service;

    service_maintainer<client, client_factory, STORAGE_SERVICE>&
        m_service_maintainer;
    storage_service_get_handler<client, STORAGE_SERVICE> m_getter;
    std::atomic<unsigned> m_round_robin = 0u;
};

} // end namespace uh::cluster
